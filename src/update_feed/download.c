#include <curl/curl.h>
#include "update_feed/update_feed.h"

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
	if (strstr(header->ptr, "ETag: ") == header->ptr) {
		char *first_quote_pos = strchr(header->ptr, '"');
		if (first_quote_pos != NULL) {
			char *second_quote_pos = strchr(first_quote_pos + 1, '"');
			if (second_quote_pos != NULL) {
				size_t new_etag_value_len = second_quote_pos - first_quote_pos - 1;
				cpyas(data->etag_header, first_quote_pos + 1, new_etag_value_len);
				INFO("Found ETag header during feed download: %s", data->etag_header->ptr);
			}
		}
	}
	free_string(header);
	return real_size;
}

static inline void
prepare_curl_for_performance(CURL *curl, const char *url, struct curl_slist *headers, struct getfeed_feed *feed, void *writedata, char *errbuf)
{
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &parse_stream_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, writedata);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &header_callback);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, feed);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10);
	// Empty string enables all supported built-in encodings
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);

	if (cfg.proxy->len != 0) {
		curl_easy_setopt(curl, CURLOPT_PROXY, cfg.proxy->ptr);
		if (cfg.proxy_auth->len != 0) {
			curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, cfg.proxy_auth->ptr);
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

	struct string *old_etag = crtss(feed->etag_header);
	if (old_etag == NULL) {
		FAIL("Not enough memory for the previous ETag header!");
		curl_easy_cleanup(curl);
		return DOWNLOAD_FAILED;
	}

	char curl_errbuf[CURL_ERROR_SIZE];
	curl_errbuf[0] = '\0';

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "A-IM: feed");
	if ((cfg.attach_if_none_match_header == true) && (feed->etag_header->len != 0)) {
		struct string *if_none_match_header = crtas("If-None-Match: \"", 16);;
		catss(if_none_match_header, feed->etag_header);
		catcs(if_none_match_header, '"');
		headers = curl_slist_append(headers, if_none_match_header->ptr);
		INFO("Attached header: %s", if_none_match_header->ptr);
		free_string(if_none_match_header);
	}

	prepare_curl_for_performance(curl, url, headers, feed, feedbuf, curl_errbuf);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		// These are just warnings because perform can fail due to lack of network connection.
		WARN("Feed update has stopped due to fail in curl request!");
		WARN("Detailed error explanation: %s", *curl_errbuf == '\0' ? curl_easy_strerror(res) : curl_errbuf);
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
		free_string(old_etag);
		return DOWNLOAD_FAILED;
	}

	long http_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	INFO("Server returned %ld HTTP response code.", http_code);

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (http_code == 304) {
		// 304 (Not Modified) response code indicates that there is
		// no need to retransmit the requested resources.
		// It is returned because server's ETag header value was
		// equal to our ETag value (supplied with If-None-Match header).
		free_string(old_etag);
		return DOWNLOAD_CANCELED;
	}

	if (cfg.respect_etag_header == true) {
		if ((feed->etag_header->len != 0) && (strcmp(old_etag->ptr, feed->etag_header->ptr) == 0)) {
			INFO("Feed update is canceled because ETag header has the same non-empty value.");
			free_string(old_etag);
			return DOWNLOAD_CANCELED;
		}
	}

	free_string(old_etag);

	if (feedbuf->len == 0) {
		return DOWNLOAD_CANCELED;
	}

	return DOWNLOAD_SUCCEEDED;
}
