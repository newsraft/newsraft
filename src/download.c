#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "feedeater.h"

static struct string *
new_string(void) {
	struct string *s = malloc(sizeof(struct string));
	if (s == NULL) return NULL;
	s->len = 0;
	s->ptr = malloc(s->len+1);
	if (s->ptr == NULL) { free(s); return NULL; }
	s->ptr[0] = '\0';
	return s;
}

static size_t
string_write_func(void *ptr, size_t size, size_t nmemb, struct string *s)
{
	size_t new_len = s->len + size * nmemb;
	s->ptr = realloc(s->ptr, new_len + 1);
	if (s->ptr == NULL) return 0;
	memcpy(s->ptr + s->len, ptr, size * nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;
	return size * nmemb;
}

struct string *
feed_download(char *url)
{
	status_write("Downloading %s", url);

	struct string *buf = new_string();
	if (buf == NULL) {
		status_write("[insufficient memory] %s", url);
		return NULL;
	}

	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10);
	/* enable all supported built-in encodings with empty string */
	/* curl 7.72.0 has: identity, deflate, gzip, br, zstd */
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_write_func);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	/*char curl_errbuf[CURL_ERROR_SIZE];*/
	/*curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);*/
	int error = curl_easy_perform(curl);

	if (error != CURLE_OK) {
		status_write("[download failed] %s", url);
		free(buf->ptr);
		free(buf);
		return NULL;
	}

	return buf;
}
