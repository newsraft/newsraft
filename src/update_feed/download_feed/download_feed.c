#include <string.h>
#include "update_feed/download_feed/download_feed.h"

static inline bool
is_last_modified_date_string_valid(const struct string *date)
{
	// WARNING: This function does not guarantee 100% validity,
	// because it checks only a few characters of the string.
	// TODO: make it check the whole thing.
	//
	// Here is an example of the Last-Modified date string: Wed, 21 Oct 2015 07:28:00 GMT
	// It must always be 29 characters long and have "GMT" at the end.
	if (date->len != 29) {
		return false;
	}
	if (strcmp(date->ptr + 26, "GMT") != 0) {
		return false;
	}
	if ((date->ptr[3] != ',') || (date->ptr[19] != ':') || (date->ptr[22] != ':')) {
		return false;
	}
	return true;
}

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
	if (strncmp(header->ptr, "ETag: ", 6) == 0) {
		char *first_quote_pos = strchr(header->ptr, '"');
		if (first_quote_pos != NULL) {
			char *second_quote_pos = strchr(first_quote_pos + 1, '"');
			if (second_quote_pos != NULL) {
				size_t new_etag_value_len = second_quote_pos - first_quote_pos - 1;
				cpyas(data->etag_header, first_quote_pos + 1, new_etag_value_len);
				INFO("Found ETag header during feed download: %s", data->etag_header->ptr);
			}
		}
	} else if (strncmp(header->ptr, "Last-Modified: ", 15) == 0) {
		cpyas(data->last_modified_header, header->ptr + 15, header->len - 15);
		trim_whitespace_from_string(data->last_modified_header);
		INFO("Found Last-Modified header during feed download: %s", data->last_modified_header->ptr);
		if (is_last_modified_date_string_valid(data->last_modified_header) == false) {
			WARN("This Last-Modified header is not valid!");
			empty_string(data->last_modified_header);
		}
	}
	free_string(header);
	return real_size;
}

static inline void
prepare_curl_for_performance(CURL *curl, const char *url, struct curl_slist *headers, struct getfeed_feed *feed, void *writedata, char *errbuf)
{
	curl_easy_setopt(curl, CURLOPT_URL, url);
	if (cfg.send_useragent_header == true) {
		curl_easy_setopt(curl, CURLOPT_USERAGENT, cfg.useragent->ptr);
	}
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

	char curl_errbuf[CURL_ERROR_SIZE];
	curl_errbuf[0] = '\0';

	struct curl_slist *headers = create_list_of_headers(feed);

	prepare_curl_for_performance(curl, url, headers, feed, feedbuf, curl_errbuf);

	CURLcode res = curl_easy_perform(curl);
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

	if (feedbuf->len == 0) {
		return DOWNLOAD_CANCELED;
	}

	return DOWNLOAD_SUCCEEDED;
}
