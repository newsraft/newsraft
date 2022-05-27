#include <string.h>
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

struct namespace_handler {
	const char *const uri;
	const size_t uri_len;
	const struct xml_element_handler *const handlers;
};

static const struct namespace_handler namespace_handlers[] = {
#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM10
	{"http://www.w3.org/2005/Atom", 27, xml_atom10_handlers},
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS20
	{"http://backend.userland.com/rss2", 32, xml_rss20_handlers},
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSSCONTENT
	{"http://purl.org/rss/1.0/modules/content/", 40, xml_rsscontent_handlers},
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_DUBLINCORE
	{"http://purl.org/dc/elements/1.1/", 32, xml_dublincore_handlers},
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_MEDIARSS
	{"http://search.yahoo.com/mrss/", 29, xml_mediarss_handlers},
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_YANDEX
	{"http://news.yandex.ru", 21, xml_yandex_handlers},
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS11
	{"http://purl.org/net/rss1.1#", 27, xml_rss11_handlers},
	{"http://purl.org/rss/1.0/", 24, xml_rss11_handlers},
	{"http://channel.netscape.com/rdf/simple/0.9/", 43, xml_rss11_handlers},
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM03
	{"http://purl.org/atom/ns#", 24, xml_atom03_handlers},
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_GEORSS
	{"http://www.georss.org/georss", 28, xml_georss_handlers},
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_GEORSS_GML
	{"http://www.opengis.net/gml", 26, xml_georss_gml_handlers},
#endif
	{NULL, 0, NULL}
};

static inline size_t
get_namespace_handler_index_by_namespace_name(const struct string *name)
{
	for (size_t i = 0; namespace_handlers[i].uri != NULL; ++i) {
		if ((name->len == namespace_handlers[i].uri_len)
				&& (memcmp(name->ptr, namespace_handlers[i].uri, name->len) == 0))
		{
			return i;
		}
	}
	return XML_FORMATS_COUNT;
}

void
parse_element_start(struct xml_data *data, const struct string *namespace, const char *name, const TidyAttr attrs)
{
	size_t handler_index;
	if (namespace == NULL) {
		handler_index = data->default_handler;
	} else {
		handler_index = get_namespace_handler_index_by_namespace_name(namespace);
	}
	if (handler_index == XML_FORMATS_COUNT) {
		return;
	}
	const struct xml_element_handler *handlers = namespace_handlers[handler_index].handlers;
	for (size_t i = 0; handlers[i].name != NULL; ++i) {
		if (strcmp(name, handlers[i].name) == 0) {
			if (handlers[i].bitpos != 0) {
				if ((data->xml_pos[handler_index] & handlers[i].bitpos) != 0) {
					return;
				}
				data->xml_pos[handler_index] |= handlers[i].bitpos;
			}
			if (handlers[i].start_handle != NULL) {
				INFO("Handling start of \"%s\" element in \"%s\" namespace.",
					name, namespace_handlers[handler_index].uri);
				handlers[i].start_handle(data, attrs);
			}
			return;
		}
	}
}

void
parse_element_end(struct xml_data *data, const struct string *namespace, const char *name, const TidyAttr attrs)
{
	size_t handler_index;
	if (namespace == NULL) {
		handler_index = data->default_handler;
	} else {
		handler_index = get_namespace_handler_index_by_namespace_name(namespace);
	}
	if (handler_index == XML_FORMATS_COUNT) {
		return;
	}
	const struct xml_element_handler *handlers = namespace_handlers[handler_index].handlers;
	for (size_t i = 0; handlers[i].name != NULL; ++i) {
		if (strcmp(name, handlers[i].name) == 0) {
			if (handlers[i].bitpos != 0) {
				if ((data->xml_pos[handler_index] & handlers[i].bitpos) == 0) {
					return;
				}
				data->xml_pos[handler_index] &= ~handlers[i].bitpos;
			}
			if (handlers[i].end_handle != NULL) {
				INFO("Handling close of \"%s\" element in \"%s\" namespace.",
					name, namespace_handlers[handler_index].uri);
				handlers[i].end_handle(data, attrs);
			}
			return;
		}
	}
}
