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

static inline void
prepare_curl_for_performance(CURL *curl, const char *url, void *writedata, char *errbuf)
{
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, writedata);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &parse_stream_callback);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10);
	// Empty string enables all supported built-in encodings
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

	if (cfg.proxy->len != 0) {
		curl_easy_setopt(curl, CURLOPT_PROXY, cfg.proxy->ptr);
		if (cfg.proxy_auth->len != 0) {
			curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, cfg.proxy_auth->ptr);
		}
	}
}

struct string *
download_feed(const char *url)
{
	CURL *curl = curl_easy_init();
	if (curl == NULL) {
		FAIL("Failed to create a curl easy handle!");
		return NULL;
	}

	struct string *feedbuf = crtes();
	if (feedbuf == NULL) {
		FAIL("Not enough memory for feed buffer!");
		curl_easy_cleanup(curl);
		return NULL;
	}

	char curl_errbuf[CURL_ERROR_SIZE];
	curl_errbuf[0] = '\0';

	prepare_curl_for_performance(curl, url, feedbuf, curl_errbuf);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		if (curl_errbuf[0] != '\0') {
			WARN("curl_easy_perform failed: %s", curl_errbuf);
		} else {
			WARN("curl_easy_perform failed: %s", curl_easy_strerror(res));
		}
		// It is just a warning because perform can fail due to lack of network connection.
		WARN("Feed update stopped due to fail in curl request!");
		free_string(feedbuf);
		curl_easy_cleanup(curl);
		return NULL;
	}

	curl_easy_cleanup(curl);

	return feedbuf;
}
