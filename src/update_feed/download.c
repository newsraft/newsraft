#include <string.h>
#include <curl/curl.h>
#include "update_feed/update_feed.h"

static struct curl_slist *
create_list_of_headers(const struct getfeed_feed *feed)
{
	struct curl_slist *headers = curl_slist_append(NULL, "A-IM: feed");
	if (headers == NULL) {
		return NULL;
	}
	INFO("Attached header - A-IM: feed");
	if ((get_cfg_bool(CFG_SEND_IF_NONE_MATCH_HEADER) == true) && (feed->http_header_etag != NULL)) {
		struct string *if_none_match_header = crtas("If-None-Match: ", 15);
		if (if_none_match_header == NULL) {
			goto error;
		}
		if (catss(if_none_match_header, feed->http_header_etag) == false) {
			free_string(if_none_match_header);
			goto error;
		}
		struct curl_slist *tmp = curl_slist_append(headers, if_none_match_header->ptr);
		free_string(if_none_match_header);
		if (tmp == NULL) {
			goto error;
		}
		headers = tmp;
		INFO("Attached header - %s", if_none_match_header->ptr);
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
				INFO("Found \"<\" character in the beginning of the stream - consider it to be XML data.");
				if (engage_xml_parser(data) == false) {
					FAIL("Failed to engage XML parser!");
					return 0;
				}
				data->media_type = MEDIA_TYPE_XML;
				break;
			} else if (contents[i] == '{') {
				INFO("Found \"{\" character in the beginning of the stream - consider it to be JSON data.");
				if (engage_json_parser(data) == false) {
					FAIL("Failed to engage JSON parser!");
					return 0;
				}
				data->media_type = MEDIA_TYPE_JSON;
				break;
			}
		}
	}
	if (data->media_type == MEDIA_TYPE_XML) {
		if (XML_Parse(data->xml_parser, contents, real_size, false) == XML_STATUS_ERROR) {
			fail_status("XML parser ran into an error: %s", XML_ErrorString(XML_GetErrorCode(data->xml_parser)));
			return 0;
		}
	} else if (data->media_type == MEDIA_TYPE_JSON) {
		yajl_status status = yajl_parse(data->json_parser, (const unsigned char *)contents, real_size);
		if (status != yajl_status_ok) {
			fail_status("JSON parser ran into an error: %s", yajl_status_to_string(status));
			return 0;
		}
	}
	return real_size;
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
	INFO("Found a header during download - %s", header->ptr);
	if (strncasecmp(header->ptr, "ETag:", 5) == 0) {
		struct string *etag = crtas(header->ptr + 5, header->len - 5);
		if (etag != NULL) {
			trim_whitespace_from_string(etag);
			crtss_or_cpyss(&feed->http_header_etag, etag);
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

static inline void
prepare_curl_for_performance(CURL *curl, const char *url, struct curl_slist *headers, struct stream_callback_data *data, char *errbuf)
{
	curl_easy_setopt(curl, CURLOPT_URL, url);
	if (get_cfg_bool(CFG_SEND_USER_AGENT_HEADER) == true) {
		const struct string *useragent = get_cfg_string(CFG_USER_AGENT);
		INFO("Attached header - User-Agent: %s", useragent->ptr);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent->ptr);
	}
	if ((get_cfg_bool(CFG_SEND_IF_MODIFIED_SINCE_HEADER) == true) && (data->feed.http_header_last_modified > 0)) {
		curl_easy_setopt(curl, CURLOPT_TIMEVALUE, data->feed.http_header_last_modified);
		curl_easy_setopt(curl, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
		INFO("Attached header - If-Modified-Since: %ld (it was converted to date string).", data->feed.http_header_last_modified);
	}
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &parse_stream_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &header_callback);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &data->feed);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, get_cfg_uint(CFG_DOWNLOAD_TIMEOUT));
	curl_easy_setopt(curl, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)get_cfg_uint(CFG_DOWNLOAD_SPEED_LIMIT) * 1024);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, get_cfg_bool(CFG_SSL_VERIFY_HOST) ? 2 : 0);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, get_cfg_bool(CFG_SSL_VERIFY_PEER) ? 1 : 0);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10);
	// Empty string enables all supported built-in encodings
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);

	const struct string *proxy = get_cfg_string(CFG_PROXY);
	if (proxy->len != 0) {
		curl_easy_setopt(curl, CURLOPT_PROXY, proxy->ptr);
		const struct string *proxy_auth = get_cfg_string(CFG_PROXY_AUTH);
		if (proxy_auth->len != 0) {
			curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, proxy_auth->ptr);
		}
	}
}

enum download_status
download_feed(const char *url, struct stream_callback_data *data)
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
	char curl_errbuf[CURL_ERROR_SIZE] = "";

	prepare_curl_for_performance(curl, url, headers, data, curl_errbuf);

	CURLcode res = curl_easy_perform(curl);
	if (data->media_type == MEDIA_TYPE_XML) {
		free_xml_parser(data);
	} else if (data->media_type == MEDIA_TYPE_JSON) {
		free_json_parser(data);
	}
	if (res != CURLE_OK) {
		// These are just warnings because perform can fail due to lack of network connection.
		WARN("Feed update has stopped due to fail in curl request!");
		WARN("Detailed error explanation: %s", *curl_errbuf == '\0' ? curl_easy_strerror(res) : curl_errbuf);
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
		return DOWNLOAD_FAILED;
	}

	long http_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	INFO("Server returned %ld HTTP response code.", http_code);

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (http_code == 304) {
		// This 304 (Not Modified) response code indicates that
		// there is no need to retransmit the requested resources.
		// There may be two reasons for this:
		// 1) server's ETag header is equal to our If-None-Match header;
		// 2) server's Last-Modified header is equal to our If-Modified-Since header.
		return DOWNLOAD_CANCELED;
	}

	return DOWNLOAD_SUCCEEDED;
}
