#include <stdlib.h>
#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

struct namespace_handler {
	const char *name;
	const size_t name_len;
	void (*parse_element_start)(struct parser_data *data, const XML_Char *name, const XML_Char **atts);
	void (*parse_element_end)(struct parser_data *data, const XML_Char *name);
};

static const struct namespace_handler namespace_handlers[] = {
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
	{"http://www.w3.org/2005/Atom", 27, &parse_atom10_element_start, &parse_atom10_element_end},
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
	{"http://purl.org/dc/elements/1.1/", 32, &parse_dc_element_start, &parse_dc_element_end},
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS10CONTENT
	{"http://purl.org/rss/1.0/modules/content/", 40, &parse_rss10content_element_start, &parse_rss10content_element_end},
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_YANDEX
	{"http://news.yandex.ru", 21, &parse_yandex_element_start, &parse_yandex_element_end},
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM03
	{"http://purl.org/atom/ns#", 24, &parse_atom03_element_start, &parse_atom03_element_end},
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS11
	{"http://purl.org/rss/1.0/", 24, &parse_rss11_element_start, &parse_rss11_element_end},
	{"http://channel.netscape.com/rdf/simple/0.9/", 43, &parse_rss11_element_start,  &parse_rss11_element_end},
	{"http://purl.org/net/rss1.1#", 27, &parse_rss11_element_start, &parse_rss11_element_end},
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
	{"http://backend.userland.com/rss2", 32, &parse_rss20_element_start, &parse_rss20_element_end},
#endif
};

int
parse_namespace_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts) {
	char *separator_pos = strchr(name, NAMESPACE_SEPARATOR);
	if (separator_pos == NULL) {
		return 1; // tag has no namespace
	}
	size_t namespace_len = separator_pos - name;
	for (size_t i = 0; i < LENGTH(namespace_handlers); ++i) {
		if ((namespace_len == namespace_handlers[i].name_len) &&
		    (memcmp(name, namespace_handlers[i].name, namespace_len) == 0))
		{
			namespace_handlers[i].parse_element_start(data, separator_pos + 1, atts);
			return 0; // success
		}
	}
	return 1; // namespace is unknown
}

int
parse_namespace_element_end(struct parser_data *data, const XML_Char *name) {
	char *separator_pos = strchr(name, NAMESPACE_SEPARATOR);
	if (separator_pos == NULL) {
		return 1; // tag has no namespace
	}
	size_t namespace_len = separator_pos - name;
	for (size_t i = 0; i < LENGTH(namespace_handlers); ++i) {
		if ((namespace_len == namespace_handlers[i].name_len) &&
		    (memcmp(name, namespace_handlers[i].name, namespace_len) == 0))
		{
			namespace_handlers[i].parse_element_end(data, separator_pos + 1);
			return 0; // success
		}
	}
	return 1; // namespace is unknown
}
