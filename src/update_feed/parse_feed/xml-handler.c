#include <string.h>
#include "update_feed/parse_feed/parse_feed.h"

struct namespace_handler {
	const char *name;
	void (*parse_element_start)(struct xml_data *data, const char *name, const TidyAttr atts);
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

static inline const char *
get_namespace_uri_of_the_tag(struct xml_data *data, const char *tag)
{
	if (data->namespaces.defaultns != NULL) {
		return data->namespaces.defaultns->ptr; // Tag is inside namespaced parent.
	}
	const char *separator_pos = strchr(tag, XML_NAMESPACE_SEPARATOR);
	if (separator_pos == NULL) {
		return NULL; // Tag has no namespace.
	}
	const size_t namespace_name_len = separator_pos - tag;
	for (size_t i = 0; i < data->namespaces.top; ++i) {
		if ((namespace_name_len == data->namespaces.buf[i].name->len) &&
		    (memcmp(tag, data->namespaces.buf[i].name->ptr, namespace_name_len) == 0))
		{
			INFO("Found URI of the \"%s\" namespace: \"%s\".", data->namespaces.buf[i].name->ptr, data->namespaces.buf[i].uri->ptr);
			return data->namespaces.buf[i].uri->ptr;
		}
	}
	return NULL;
}

bool
parse_namespace_element_start(struct xml_data *data, const char *name, const TidyAttr atts)
{
	const char *namespace_uri = get_namespace_uri_of_the_tag(data, name);
	if (namespace_uri == NULL) {
		return false; // Namespace is unknown.
	}
	for (size_t i = 0; i < COUNTOF(namespace_handlers); ++i) {
		if (strcmp(namespace_uri, namespace_handlers[i].name) == 0) {
			const char *sep = strchr(name, XML_NAMESPACE_SEPARATOR);
			namespace_handlers[i].parse_element_start(data, sep == NULL ? name : sep + 1, atts);
			return true; // Success.
		}
	}
	return false; // Namespace is unknown.
}

bool
parse_namespace_element_end(struct xml_data *data, const char *name)
{
	const char *namespace_uri = get_namespace_uri_of_the_tag(data, name);
	if (namespace_uri == NULL) {
		return false; // Namespace is unknown.
	}
	for (size_t i = 0; i < COUNTOF(namespace_handlers); ++i) {
		if (strcmp(namespace_uri, namespace_handlers[i].name) == 0) {
			const char *sep = strchr(name, XML_NAMESPACE_SEPARATOR);
			namespace_handlers[i].parse_element_end(data, sep == NULL ? name : sep + 1);
			return true; // Success.
		}
	}
	return false; // Namespace is unknown.
}
