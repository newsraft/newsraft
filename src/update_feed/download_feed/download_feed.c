#include <string.h>
#include "update_feed/download_feed/download_feed.h"

static size_t
parse_stream_callback(char *contents, size_t length, size_t nmemb, struct string *data)
{
	size_t real_size = length * nmemb;
	if (catas(data, contents, real_size) == false) {
		FAIL("Not enough memory for downloading a feed!");
	}
	return real_size;
}

static size_t
header_callback(char *buffer, size_t size, size_t nitems, struct getfeed_feed *data)
{
	size_t real_size = nitems * size;
	struct string *header = crtas(buffer, real_size);
	if (header == NULL) {
		return 0;
	}
	trim_whitespace_from_string(header);
	INFO("Found a header during loading - \"%s\".", header->ptr);
	if (strncasecmp(header->ptr, "Expires: ", 9) == 0) {
		if (get_cfg_bool(CFG_RESPECT_EXPIRES_HEADER) == true) {
			time_t expire_date = curl_getdate(header->ptr + 9, NULL);
			if (expire_date > 0) {
				if ((data->previous_download_date > 0) && (data->previous_download_date < expire_date)) {
					INFO("Closing connection because previous download date is less than expiration date.");
					// Set this variable to -1 to indicate that curl was stopped because of this ^.
					data->previous_download_date = -1;
					free_string(header);
					return 0;
				}
			} else {
				FAIL("Curl failed to parse date string!");
			}
		}
	} else if (strncasecmp(header->ptr, "ETag: ", 6) == 0) {
		char *first_quote_pos = strchr(header->ptr, '"');
		if (first_quote_pos != NULL) {
			char *second_quote_pos = strchr(first_quote_pos + 1, '"');
			if (second_quote_pos != NULL) {
				size_t new_etag_value_len = second_quote_pos - first_quote_pos - 1;
				cpyas(data->http_header_etag, first_quote_pos + 1, new_etag_value_len);
			}
		} else {
			cpyas(data->http_header_etag, header->ptr + 6, header->len - 6);
			trim_whitespace_from_string(data->http_header_etag);
		}
	} else if (strncasecmp(header->ptr, "Last-Modified: ", 15) == 0) {
		time_t date = curl_getdate(header->ptr + 15, NULL);
		if (date > 0) {
			data->http_header_last_modified = date;
		} else {
			FAIL("Curl failed to parse date string!");
		}
	}
	free_string(header);
	return real_size;
}

static inline void
prepare_curl_for_performance(CURL *curl, const char *url, struct curl_slist *headers, struct getfeed_feed *feed, void *writedata, char *errbuf)
{
	curl_easy_setopt(curl, CURLOPT_URL, url);
	if (get_cfg_bool(CFG_SEND_USER_AGENT_HEADER) == true) {
		const struct string *useragent = get_cfg_string(CFG_USER_AGENT);
		INFO("Attached user-agent: %s", useragent->ptr);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent->ptr);
	}
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &parse_stream_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, writedata);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &header_callback);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, feed);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, get_cfg_uint(CFG_DOWNLOAD_TIMEOUT));
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
download_feed(const char *url, struct getfeed_feed *feed, struct string *feedbuf)
{
	CURL *curl = curl_easy_init();
	if (curl == NULL) {
		FAIL("Failed to create a curl easy handle!");
		return DOWNLOAD_FAILED;
	}

	char curl_errbuf[CURL_ERROR_SIZE];
	curl_errbuf[0] = '\0';

	struct curl_slist *headers = create_list_of_headers(feed);

	prepare_curl_for_performance(curl, url, headers, feed, feedbuf, curl_errbuf);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		if (feed->previous_download_date == -1) {
			// See header_callback function to understand what is going on here.
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			INFO("Download is canceled because the information on the server hasn't expired yet.");
			return DOWNLOAD_CANCELED;
		} else {
			// These are just warnings because perform can fail due to lack of network connection.
			WARN("Feed update has stopped due to fail in curl request!");
			WARN("Detailed error explanation: %s", *curl_errbuf == '\0' ? curl_easy_strerror(res) : curl_errbuf);
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			return DOWNLOAD_FAILED;
		}
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

	if (feedbuf->len == 0) {
		return DOWNLOAD_CANCELED;
	}

	return DOWNLOAD_SUCCEEDED;
}
