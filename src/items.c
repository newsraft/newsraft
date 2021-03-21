#include <unistd.h>
#include <ncurses.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include "feedeater.h"

struct string {
	char *ptr;
	size_t len;
};

struct string *
new_string(void) {
	struct string *s = malloc(sizeof(struct string));
	if (s == NULL) return NULL;
	s->len = 0;
	s->ptr = malloc(s->len+1);
	if (s->ptr == NULL) { free(s); return NULL; }
	s->ptr[0] = '\0';
	return s;
}

size_t
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

static struct string *
feed_download(char *url)
{
	wclear(status_win);
	mvwprintw(status_win, 0, 0, "Downloading %s", url);
	wrefresh(status_win);

	struct string *buf = new_string();
	if (buf == NULL) {
		wclear(status_win);
		mvwprintw(status_win, 0, 0, "Failed to allocate memory for %s", url);
		wrefresh(status_win);
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
		wclear(status_win);
		mvwprintw(status_win, 0, 0, "Failed to retrieve %s", url);
		wrefresh(status_win);
		free(buf->ptr);
		free(buf);
		return NULL;
	}

	return buf;
}

void
feed_process(struct string *buf, char *url)
{
	xmlDocPtr doc = xmlReadMemory(buf->ptr, buf->len, url, NULL, XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
	if (doc == NULL) {
		STATUS_MSG("Failed to parse %s contents", url);
		return;
	}
	xmlNodePtr node = xmlDocGetRootElement(doc);
	if (node == NULL) {
		STATUS_MSG("Empty - %s", url);
		return;
	}
	if (node->name == NULL || node->type != XML_ELEMENT_NODE) {
		STATUS_MSG("Invalid feed in %s", url);
		return;
	}
	if (strcmp((char *)node->name, "rss") == 0) {
		char *version = (char *)xmlGetProp(node, (xmlChar *)"version");
		STATUS_MSG("RSS VERSION: %s", version);
	} else if (strcmp((const char*)node->name, "RDF")) {
		STATUS_MSG("RSS VERSION: 1.0");
	} else if (strcmp((char *)node->name, "feed") == 0) {
		char *version = (char *)xmlGetProp(node, (xmlChar *)"version");
		STATUS_MSG("ATOM VERSION: %s", version);
	}
	xmlFreeNode(node);
	//xmlFreeDoc(doc);
}

void
feed_reload(char *url)
{
	struct string *buf = feed_download(url);
	if (buf == NULL) return;
	feed_process(buf, url);
	free(buf);
}

void
feed_reload_all(void)
{
	//under construction
	return;
}

void
feed_view(char *url)
{
	//under construction
	return;
}
