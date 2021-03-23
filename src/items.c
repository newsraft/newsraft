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
#include "parsers/parsers.h"

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
	status_write("Downloading %s", url);

	struct string *buf = new_string();
	if (buf == NULL) {
		status_write("Failed to allocate memory for %s", url);
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
		status_write("Failed to retrieve %s", url);
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
		status_write("Unable to parse: %s", url);
		return;
	}
	xmlNodePtr node = xmlDocGetRootElement(doc);
	if (node == NULL) {
		status_write("Empty contents: %s", url);
		return;
	}
	if ((node->name == NULL) || (node->type != XML_ELEMENT_NODE)) {
		status_write("Unknown format: %s", url);
		return;
	}
	if (strcmp((char *)node->name, "rss") == 0) {
		char *version = (char *)xmlGetProp(node, (xmlChar *)"version");
		if (strcmp(version, "2.0") == 0) {
			parse_rss20(node);
		} else if (strcmp(version, "1.1") == 0) {
			parse_rss11(node);
		} else if (strcmp(version, "1.0") == 0) {
			parse_rss10(node);
		} else if (strcmp(version, "0.94") == 0) {
			parse_rss094(node);
		} else if (strcmp(version, "0.92") == 0) {
			parse_rss092(node);
		} else if (strcmp(version, "0.91") == 0) {
			parse_rss091(node);
		} else if (strcmp(version, "0.90") == 0) {
			parse_rss090(node);
		} else {
			status_write("Unsupported RSS version (%s): %s", version, url);
		}
	} else if (strcmp((char *)node->name, "feed") == 0) {
		char *version = (char *)xmlGetProp(node, (xmlChar *)"version");
		status_write("ATOM VERSION: %s", version);
	} else if (strcmp((const char*)node->name, "RDF") == 0) {
		parse_rss10(node);
	} else {
		status_write("Invalid format: %s", url);
	}
	xmlFreeNode(node);
	//xmlFreeDoc(doc);
}

void
feed_reload(char *url)
{
	struct string *buf = feed_download(url);
	if (buf == NULL) return;
	if (buf->ptr == NULL) { free(buf); return; }
	feed_process(buf, url);
	free(buf->ptr);
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
