#include <string.h>
#include <strings.h>
#include <curl/curl.h>
#include "update_feed/update_feed.h"

// Note to the future.
// Sending "A-IM: feed" header is essential, since some servers in the presence
// of this header and other headers indicating that the feed was already been
// downloaded before, may limit themselves to sending only those entries that we
// do not have; and those entries that were downloaded earlier will not be sent.
// Thus resources and traffic will be saved. They call this "delta update".

// Unknown type must have 0 value!
enum {
	MEDIA_TYPE_UNKNOWN = 0,
	MEDIA_TYPE_XML,
	MEDIA_TYPE_JSON,
};

static struct curl_slist *
create_list_of_headers(const struct getfeed_feed *feed)
{
	struct curl_slist *headers = curl_slist_append(NULL, "A-IM: feed");
	if (headers == NULL) {
		return NULL;
	}
	INFO("Attached header - A-IM: feed");
	if (feed->http_header_etag != NULL) {
		struct string *if_none_match_header = crtas("If-None-Match: ", 15);
		if (if_none_match_header == NULL) {
			goto error;
		}
		if (catss(if_none_match_header, feed->http_header_etag) == false) {
			free_string(if_none_match_header);
			goto error;
		}
		struct curl_slist *tmp = curl_slist_append(headers, if_none_match_header->ptr);
		if (tmp == NULL) {
			free_string(if_none_match_header);
			goto error;
		}
		headers = tmp;
		INFO("Attached header - %s", if_none_match_header->ptr);
		free_string(if_none_match_header);
	}
	return headers;
error:
	curl_slist_free_all(headers);
	return NULL;
}

static size_t
parse_stream_callback(char *contents, size_t length, size_t nmemb, void *userdata)
{
	struct stream_callback_data *data = userdata;
	const size_t real_size = length * nmemb;
	if (data->media_type == MEDIA_TYPE_UNKNOWN) {
		for (size_t i = 0; i < real_size; ++i) {
			if (contents[i] == '<') {
				INFO("The stream has \"<\" character in the beginning - engaging XML parser.");
				if (setup_xml_parser(data) == false) {
					FAIL("Failed to setup XML parser!");
					return CURL_WRITEFUNC_ERROR;
				}
				data->media_type = MEDIA_TYPE_XML;
				break;
			} else if (contents[i] == '{') {
				INFO("The stream has \"{\" character in the beginning - engaging JSON parser.");
				if (setup_json_parser(data) == false) {
					FAIL("Failed to setup JSON parser!");
					return CURL_WRITEFUNC_ERROR;
				}
				data->media_type = MEDIA_TYPE_JSON;
				break;
			}
		}
	}
	if (data->media_type == MEDIA_TYPE_XML) {
		if (XML_Parse(data->xml_parser, contents, real_size, false) != XML_STATUS_OK) {
			fail_status("XML parser ran into an error: %s", XML_ErrorString(XML_GetErrorCode(data->xml_parser)));
			return CURL_WRITEFUNC_ERROR;
		}
	} else if (data->media_type == MEDIA_TYPE_JSON) {
		yajl_status status = yajl_parse(data->json_parser, (const unsigned char *)contents, real_size);
		if (status != yajl_status_ok) {
			fail_status("JSON parser ran into an error: %s", yajl_status_to_string(status));
			return CURL_WRITEFUNC_ERROR;
		}
	}
	return they_want_us_to_terminate != true ? real_size : CURL_WRITEFUNC_ERROR;
}

static size_t
header_callback(char *contents, size_t length, size_t nmemb, void *userdata)
{
	struct getfeed_feed *feed = userdata;
	const size_t real_size = nmemb * length;
	struct string *header = crtas(contents, real_size);
	if (header == NULL) {
		return 0;
	}
	trim_whitespace_from_string(header);
	INFO("Received header - %s", header->ptr);
	if (strncasecmp(header->ptr, "ETag:", 5) == 0) {
		struct string *etag = crtas(header->ptr + 5, header->len - 5);
		if (etag != NULL) {
			trim_whitespace_from_string(etag);
			cpyss(&feed->http_header_etag, etag);
			free_string(etag);
		}
	} else if (strncasecmp(header->ptr, "Last-Modified: ", 15) == 0) {
		feed->http_header_last_modified = curl_getdate(header->ptr + 15, NULL);
		if (feed->http_header_last_modified < 0) {
			FAIL("Curl failed to parse date string!");
			feed->http_header_last_modified = 0;
		}
	} else if (strncasecmp(header->ptr, "Expires: ", 9) == 0) {
		feed->http_header_expires = curl_getdate(header->ptr + 9, NULL);
		if (feed->http_header_expires < 0) {
			FAIL("Curl failed to parse date string!");
			feed->http_header_expires = 0;
		}
	}
	free_string(header);
	return real_size;
}

static inline struct string *
get_proxy_auth_info_encoded(const char *user, const char *password)
{
	if ((user != NULL) && (password != NULL)) {
		struct string *result = crtas(user, strlen(user));
		if (result != NULL) {
			if (catcs(result, ':') == true) {
				if (catas(result, password, strlen(password)) == true) {
					return result;
				}
			}
			free_string(result);
		}
	}
	return NULL;
}

static inline bool
prepare_curl_for_performance(CURL *curl, struct feed_entry *feed, struct curl_slist *headers, struct stream_callback_data *data, char *errbuf)
{
	curl_easy_setopt(curl, CURLOPT_URL, feed->link->ptr);
	if (get_cfg_bool(&feed->cfg, CFG_SEND_USER_AGENT_HEADER) == true) {
		const struct string *useragent = get_cfg_string(&feed->cfg, CFG_USER_AGENT);
		INFO("Attached header - User-Agent: %s", useragent->ptr);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent->ptr);
	}
	if (data->feed.http_header_last_modified > 0) {
		curl_easy_setopt(curl, CURLOPT_TIMEVALUE, data->feed.http_header_last_modified);
		curl_easy_setopt(curl, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
		INFO("Attached header - If-Modified-Since: %" PRId64 " (it was converted to date string).", data->feed.http_header_last_modified);
	}
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &parse_stream_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &header_callback);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &data->feed);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, get_cfg_uint(&feed->cfg, CFG_DOWNLOAD_TIMEOUT));
	curl_easy_setopt(curl, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)get_cfg_uint(&feed->cfg, CFG_DOWNLOAD_SPEED_LIMIT) * 1024);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); // Enable all supported built-in encodings.
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);

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
				return false;
			}
			curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, auth->ptr);
			free_string(auth);
		}
	}
	return true;
}

download_status
download_feed(struct feed_entry *feed, struct stream_callback_data *data)
{
	CURL *curl = curl_easy_init();
	if (curl == NULL) {
		FAIL("Failed to create curl handle!");
		return DOWNLOAD_FAILED;
	}
	struct curl_slist *headers = create_list_of_headers(&data->feed);
	if (headers == NULL) {
		FAIL("Failed to create headers list!");
		curl_easy_cleanup(curl);
		return DOWNLOAD_FAILED;
	}
	char curl_errbuf[CURL_ERROR_SIZE] = {0};
	if (prepare_curl_for_performance(curl, feed, headers, data, curl_errbuf) == false) {
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
		return DOWNLOAD_FAILED;
	}

	CURLcode res = curl_easy_perform(curl);
	if (data->media_type == MEDIA_TYPE_XML) {
		free_xml_parser(data);
	} else if (data->media_type == MEDIA_TYPE_JSON) {
		free_json_parser(data);
	}

	long response = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
	INFO("Curl response code: %ld", response);

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (response == 304) {
		// 304 (Not Modified) response code indicates that
		// there is no need to retransmit the requested resources.
		// There may be two reasons for this:
		// 1) server's ETag header is equal to our If-None-Match header;
		// 2) server's Last-Modified header is equal to our If-Modified-Since header.
		return DOWNLOAD_CANCELED;
	} else if (response == 429) {
		info_status("The server rejected the download because updates are too frequent.");
		return DOWNLOAD_CANCELED;
	}

	if (res != CURLE_OK) {
		fail_status("Curl error: %s; %s", curl_easy_strerror(res), curl_errbuf);
		if (response != 0) {
			fail_status("The server which keeps the feed returned %ld status code!", response);
		}
		return DOWNLOAD_FAILED;
	}

	return DOWNLOAD_SUCCEEDED;
}
