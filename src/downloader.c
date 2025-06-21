#include <signal.h>
#include <string.h>
#include <strings.h>
#include "newsraft.h"

static CURLM *multi;

static struct curl_slist *
create_list_of_headers(const struct feed_update_state *data)
{
	// A-IM header is a hint for servers that they can send only a part of data,
	// often known as "delta". Server will know which part of data to send
	// exactly by analyzing our If-None-Match header. See RFC3229 for more info.
	struct curl_slist *headers = curl_slist_append(NULL, "A-IM: feed");
	if (headers == NULL) {
		return NULL;
	}
	INFO("Attached header - A-IM: feed");

	if (get_cfg_bool(&data->feed_entry->cfg, CFG_SEND_IF_NONE_MATCH_HEADER) == true) {
		struct string *etag = db_get_string_from_feed_table(data->feed_entry->link, "http_header_etag", 16);
		if (etag != NULL) {
			struct string *if_none_match = crtas("If-None-Match: ", 15);
			if (if_none_match == NULL) {
				free_string(if_none_match);
				free_string(etag);
				goto error;
			}
			catss(if_none_match, etag);
			free_string(etag);
			struct curl_slist *tmp = curl_slist_append(headers, if_none_match->ptr);
			if (tmp == NULL) {
				free_string(if_none_match);
				goto error;
			}
			headers = tmp;
			INFO("Attached header - %s", if_none_match->ptr);
			free_string(if_none_match);
		}
	}

	return headers;
error:
	curl_slist_free_all(headers);
	return NULL;
}

static size_t
parse_stream_callback(char *contents, size_t length, size_t nmemb, void *userdata)
{
	struct feed_update_state *data = userdata;
	const size_t real_size = length * nmemb;
	if (data->media_type == MEDIA_TYPE_UNKNOWN) {
		for (size_t i = 0; i < real_size; ++i) {
			if (contents[i] == '<') {
				INFO("The stream has \"<\" character in the beginning - engaging XML parser.");
				if (setup_xml_parser(data) == false) {
					FAIL("Failed to setup XML parser!");
					return CURL_WRITEFUNC_ERROR;
				}
				break;
			} else if (contents[i] == '{') {
				INFO("The stream has \"{\" character in the beginning - engaging JSON parser.");
				setup_json_parser(data);
				break;
			}
		}
	}
	if (data->media_type == MEDIA_TYPE_XML) {
		if (XML_Parse(data->xml_parser, contents, real_size, XML_FALSE) != XML_STATUS_OK) {
			str_appendf(data->new_errors, "XML parser failed: %s\n", XML_ErrorString(XML_GetErrorCode(data->xml_parser)));
			return CURL_WRITEFUNC_ERROR;
		}
	} else if (data->media_type == MEDIA_TYPE_JSON) {
		catas(data->text, contents, real_size);
	}
	return they_want_us_to_stop ? CURL_WRITEFUNC_ERROR : real_size;
}

static size_t
header_callback(char *contents, size_t length, size_t nmemb, void *userdata)
{
	struct getfeed_feed *feed = userdata;
	const size_t real_size = nmemb * length;

	size_t header_name_len = 0;
	for (size_t i = 0; i < real_size; ++i) {
		if (contents[i] == ':') {
			header_name_len = i;
			break;
		}
	}
	if (header_name_len == 0) {
		return real_size; // Ignore invalid headers.
	}

	struct string *header_name = crtas(contents, header_name_len);
	struct string *header_value = crtas(contents + header_name_len + 1, real_size - header_name_len - 1);
	if (header_name == NULL || header_value == NULL) {
		free_string(header_name);
		free_string(header_value);
		return CURL_WRITEFUNC_ERROR;
	}
	trim_whitespace_from_string(header_name);
	trim_whitespace_from_string(header_value);

	if (strcasecmp(header_name->ptr, "ETag") == 0) {
		INFO("Got ETag header > %s", header_value->ptr);
		cpyss(&feed->http_header_etag, header_value);
	} else if (strcasecmp(header_name->ptr, "Last-Modified") == 0) {
		INFO("Got Last-Modified header > %s", header_value->ptr);
		feed->http_header_last_modified = curl_getdate(header_value->ptr, NULL);
		if (feed->http_header_last_modified < 0) {
			FAIL("Curl failed to parse date string!");
			feed->http_header_last_modified = 0;
		}
	} else if (strcasecmp(header_name->ptr, "Expires") == 0) {
		INFO("Got Expires header > %s", header_value->ptr);
		feed->http_header_expires = curl_getdate(header_value->ptr, NULL);
		if (feed->http_header_expires < 0) {
			FAIL("Curl failed to parse date string!");
			feed->http_header_expires = 0;
		}
	} else {
		INFO("Got needless header > %s: %s", header_name->ptr, header_value->ptr);
	}

	free_string(header_name);
	free_string(header_value);

	return they_want_us_to_stop ? CURL_WRITEFUNC_ERROR : real_size;
}

static inline struct string *
get_proxy_auth_info_encoded(const char *user, const char *password)
{
	if (user == NULL || password == NULL) {
		return NULL;
	}
	struct string *result = crtas(user, strlen(user));
	catcs(result, ':');
	catas(result, password, strlen(password));
	return result;
}

static inline bool
prepare_feed_update_state_for_download(struct feed_update_state *data)
{
	struct feed_entry *feed = data->feed_entry;

	if (get_cfg_bool(&feed->cfg, CFG_RESPECT_EXPIRES_HEADER) == true) {
		int64_t expires_date = db_get_date_from_feeds_table(feed->link, "http_header_expires", 19);
		if (expires_date < 0) {
			FAIL("Skipping %s because its HTTP header is invalid", feed->link->ptr);
			goto fail;
		} else if (expires_date > 0 && feed->update_date < expires_date) {
			INFO("Skipping %s because its HTTP header is not expired yet", feed->link->ptr);
			goto cancel;
		}
	}

	if (get_cfg_bool(&feed->cfg, CFG_RESPECT_TTL_ELEMENT) == true) {
		int64_t download_date = db_get_date_from_feeds_table(feed->link, "download_date", 13);
		int64_t ttl = db_get_date_from_feeds_table(feed->link, "time_to_live", 12);
		if (download_date < 0 || ttl < 0) {
			FAIL("Skipping %s because its ttl element is invalid", feed->link->ptr);
			goto fail;
		} else if (download_date > 0 && ttl > 0 && (download_date + ttl) > feed->update_date) {
			INFO("Skipping %s because its ttl element is not expired yet", feed->link->ptr);
			goto cancel;
		}
	}

	CURL *curl = curl_easy_init();
	data->curl = curl;
	if (data->curl == NULL) {
		goto fail;
	}

	data->download_headers = create_list_of_headers(data);
	if (data->download_headers == NULL) {
		goto fail;
	}

	curl_easy_setopt(curl, CURLOPT_URL, feed->link->ptr);
	curl_easy_setopt(curl, CURLOPT_PRIVATE, data);
	const struct string *useragent = get_cfg_string(&feed->cfg, CFG_USER_AGENT);
	if (useragent->len > 0) {
		INFO("Attached header - User-Agent: %s", useragent->ptr);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent->ptr);
	}
	if (get_cfg_bool(&feed->cfg, CFG_SEND_IF_MODIFIED_SINCE_HEADER) == true) {
		int64_t last_modified = db_get_date_from_feeds_table(feed->link, "http_header_last_modified", 25);
		if (last_modified > 0) {
			curl_easy_setopt(curl, CURLOPT_TIMEVALUE, last_modified);
			curl_easy_setopt(curl, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
			INFO("Attached header - If-Modified-Since: %" PRId64 " (it was converted to date string).", last_modified);
		} else if (last_modified < 0) {
			FAIL("Skipping %s because its http_header_last_modified is invalid", feed->link->ptr);
			goto fail;
		}
	}
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, data->download_headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &parse_stream_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &header_callback);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &data->feed);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, get_cfg_uint(&feed->cfg, CFG_DOWNLOAD_TIMEOUT));
	curl_easy_setopt(curl, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)get_cfg_uint(&feed->cfg, CFG_DOWNLOAD_SPEED_LIMIT) * 1024);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, data->curl_error);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); // Enable all supported built-in encodings.
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

	FILE *log_stream = log_get_stream();
	if (log_stream) {
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_STDERR, log_stream);
	} else {
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
	}

	const struct string *proxy = get_cfg_string(&feed->cfg, CFG_PROXY);
	if (proxy->len != 0) {
		curl_easy_setopt(curl, CURLOPT_PROXY, proxy->ptr);
		const struct string *user_str = get_cfg_string(&feed->cfg, CFG_PROXY_USER);
		const struct string *password_str = get_cfg_string(&feed->cfg, CFG_PROXY_PASSWORD);
		if ((user_str->len != 0) && (password_str->len != 0)) {
			char *user = curl_easy_escape(curl, user_str->ptr, user_str->len);
			char *password = curl_easy_escape(curl, password_str->ptr, password_str->len);
			struct string *auth = get_proxy_auth_info_encoded(user, password);
			curl_free(user);
			curl_free(password);
			if (auth == NULL) {
				goto fail;
			}
			curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, auth->ptr);
			free_string(auth);
		}
	}

	return true;
cancel:
	data->is_canceled = true;
	return false;
fail:
	data->is_failed = true;
	return false;
}

static bool
engage_with_not_downloaded_feed(struct feed_update_state *data)
{
	if (data->is_finished == false
		&& data->is_in_progress == false
		&& data->feed_entry->link->ptr[0] != '$')
	{
		data->is_in_progress = true;
		return true;
	}
	return false;
}

void *
downloader_worker(void *dummy)
{
	(void)dummy;
	int still_running = 0; // Number of active handles
	CURLMsg *msg;
	int msgs_left = 0;
	uint64_t perform_count = 0;

	while (they_want_us_to_stop == false) {

		struct feed_update_state *target = queue_pull(&engage_with_not_downloaded_feed);

		if (target != NULL) {
			if (prepare_feed_update_state_for_download(target) == true) {
				curl_multi_add_handle(multi, target->curl);
				target->curl_handle_added_to_multi = true;
				INFO("Populated curl multi handle with %s", target->feed_entry->link->ptr);
			}
		} else if (still_running == 0) {
			threads_take_a_nap(NEWSRAFT_THREAD_DOWNLOAD);
		} else {
			// This one is waken up with curl_multi_wakeup()
			curl_multi_poll(multi, NULL, 0, 1 /* ms */, NULL);
		}

		perform_count += 1;
		CURLMcode status = curl_multi_perform(multi, &still_running);
		if (status != CURLM_OK) {
			FAIL("Got an error while performing on multi handle: %s", curl_multi_strerror(status));
		}

		while ((msg = curl_multi_info_read(multi, &msgs_left)) != NULL) {
			if (msg->msg != CURLMSG_DONE) {
				continue;
			}

			INFO("Downloader stats: %" PRIu64 " curl performances, %d still running, %d msgs left",
				perform_count,
				still_running,
				msgs_left
			);

			struct feed_update_state *data = NULL;
			curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &data); // Set with CURLOPT_PRIVATE
			curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &data->http_response_code);
			INFO("HTTP response code: %ld", data->http_response_code);

			if (data->http_response_code == NEWSRAFT_HTTP_NOT_MODIFIED) {
				// Server says that our stored content is up to date. It knows it based on:
				// 1) server's ETag header is equal to our If-None-Match header; and/or
				// 2) server's Last-Modified header is equal to our If-Modified-Since header.
				data->is_canceled = true;
			} else if (data->http_response_code == NEWSRAFT_HTTP_TOO_MANY_REQUESTS) {
				WARN("The server rejected the download because updates are too frequent.");
				data->is_canceled = true;
			}

			if (msg->data.result != CURLE_OK) {
				str_appendf(data->new_errors, "curl failed: %s, %s\n", curl_easy_strerror(msg->data.result), data->curl_error);
				if (data->http_response_code != 0) {
					str_appendf(data->new_errors, "The server which keeps the feed returned %ld status code!\n", data->http_response_code);
				}
				data->is_failed = true;
			}

			// Final parsing call
			if (data->is_failed == false && data->is_canceled == false) {
				if (data->media_type == MEDIA_TYPE_XML) {
					if (XML_Parse(data->xml_parser, NULL, 0, XML_TRUE) != XML_STATUS_OK) {
						data->is_failed = true;
					}
				} else if (data->media_type == MEDIA_TYPE_JSON) {
					if (!newsraft_json_parse(data, data->text->ptr, data->text->len)) {
						data->is_failed = true;
					}
				}
			}

			data->is_downloaded = true;
			threads_wake_up(NEWSRAFT_THREAD_DBWRITER);
		}

	}

	return NULL;
}

void
downloader_curl_wakeup(void)
{
	curl_multi_wakeup(multi);
}

void
remove_downloader_handle(struct feed_update_state *data)
{
	if (data->curl_handle_added_to_multi) {
		curl_multi_remove_handle(multi, data->curl);
	}
}

bool
curl_init(void)
{
	curl_version_info_data *ver = curl_version_info(CURLVERSION_NOW);
	INFO("libcurl AsynchDNS feature: %s", ver->features & CURL_VERSION_ASYNCHDNS ? "enabled" : "disabled");

	if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
		write_error("Failed to initialize curl!\n");
		return false;
	}
	// Initialize "multi stack". This is the way to do asynchronous
	// downloads in CURL with shared connection and DNS cache.
	multi = curl_multi_init();
	if (multi == NULL) {
		write_error("Failed to initialize curl multi stack!\n");
		return false;
	}
	curl_multi_setopt(multi, CURLMOPT_MAX_TOTAL_CONNECTIONS, get_cfg_uint(NULL, CFG_DOWNLOAD_MAX_CONNECTIONS));
	curl_multi_setopt(multi, CURLMOPT_MAX_HOST_CONNECTIONS, get_cfg_uint(NULL, CFG_DOWNLOAD_MAX_HOST_CONNECTIONS));
	return true;
}

void
curl_stop(void)
{
	queue_destroy();
	curl_multi_cleanup(multi);
	curl_global_cleanup();
}
