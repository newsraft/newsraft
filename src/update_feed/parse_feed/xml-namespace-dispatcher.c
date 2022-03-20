#include <string.h>
#include "update_feed/parse_feed/parse_feed.h"

struct namespace_handler {
	const char *uri;
	void (*parse_element_start)(struct xml_data *data, const char *name, const TidyAttr attrs);
	void (*parse_element_end)(struct xml_data *data, const char *name);
};

static const struct namespace_handler namespace_handlers[] = {
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
	{"http://www.w3.org/2005/Atom", &parse_atom10_element_start, &parse_atom10_element_end},
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
	{"http://purl.org/dc/elements/1.1/", &parse_dc_element_start, &parse_dc_element_end},
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS10CONTENT
	{"http://purl.org/rss/1.0/modules/content/", &parse_rss10content_element_start, &parse_rss10content_element_end},
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_YANDEX
	{"http://news.yandex.ru", &parse_yandex_element_start, &parse_yandex_element_end},
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM03
	{"http://purl.org/atom/ns#", &parse_atom03_element_start, &parse_atom03_element_end},
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS11
	{"http://purl.org/rss/1.0/", &parse_rss11_element_start, &parse_rss11_element_end},
	{"http://channel.netscape.com/rdf/simple/0.9/", &parse_rss11_element_start,  &parse_rss11_element_end},
	{"http://purl.org/net/rss1.1#", &parse_rss11_element_start, &parse_rss11_element_end},
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
	{"http://backend.userland.com/rss2", &parse_rss20_element_start, &parse_rss20_element_end},
#endif
};

void
parse_namespace_element_start(struct xml_data *data, const struct string *namespace, const char *name, const TidyAttr attrs)
{
	for (size_t i = 0; i < COUNTOF(namespace_handlers); ++i) {
		if (strcmp(namespace->ptr, namespace_handlers[i].uri) == 0) {
			namespace_handlers[i].parse_element_start(data, name, attrs);
			break;
		}
	}
}

void
parse_namespace_element_end(struct xml_data *data, const struct string *namespace, const char *name)
{
	for (size_t i = 0; i < COUNTOF(namespace_handlers); ++i) {
		if (strcmp(namespace->ptr, namespace_handlers[i].uri) == 0) {
			namespace_handlers[i].parse_element_end(data, name);
			break;
		}
	}
}
