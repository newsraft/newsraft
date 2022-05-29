#include <string.h>
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

#define XML_NAMESPACE_SEPARATOR ' '

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
get_namespace_handler_index_by_namespace_name(const char *ns, size_t ns_len)
{
	for (size_t i = 0; namespace_handlers[i].uri != NULL; ++i) {
		if ((ns_len == namespace_handlers[i].uri_len)
				&& (memcmp(ns, namespace_handlers[i].uri, ns_len) == 0))
		{
			return i;
		}
	}
	return XML_FORMATS_COUNT;
}

static void
start_element_handler(void *userData, const XML_Char *name, const XML_Char **atts)
{
	struct stream_callback_data *data = userData;
	++(data->depth);
	empty_string(data->value);
	const char *tag;
	size_t handler_index;
	const char *sep_pos = strchr(name, XML_NAMESPACE_SEPARATOR);
	if (sep_pos != NULL) {
		tag = sep_pos + 1;
		handler_index = get_namespace_handler_index_by_namespace_name(name, sep_pos - name);
	} else {
		tag = name;
		handler_index = data->xml_format;
	}
	INFO("Element start: %s (%s)", tag, name);
	if (handler_index != XML_FORMATS_COUNT) {
		const struct xml_element_handler *handlers = namespace_handlers[handler_index].handlers;
		for (size_t i = 0; handlers[i].name != NULL; ++i) {
			if (strcmp(tag, handlers[i].name) == 0) {
				if (handlers[i].bitpos != 0) {
					if ((data->xml_pos[handler_index] & handlers[i].bitpos) != 0) {
						return;
					}
					data->xml_pos[handler_index] |= handlers[i].bitpos;
				}
				if (handlers[i].start_handle != NULL) {
					handlers[i].start_handle(data, atts);
				}
				return;
			}
		}
	}
	if (data->xml_format != XML_FORMATS_COUNT) {
		return;
	}
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS20
	if ((data->depth == 1) && (strcmp(name, "rss") == 0)) {
		const char *version = get_value_of_attribute_key(atts, "version");
		if ((version != NULL) &&
			((strcmp(version, "2.0") == 0) ||
			(strcmp(version, "0.94") == 0) ||
			(strcmp(version, "0.93") == 0) ||
			(strcmp(version, "0.92") == 0) ||
			(strcmp(version, "0.91") == 0)))
		{
			data->xml_format = RSS20_FORMAT;
		}
	}
#endif
}

static void
end_element_handler(void *userData, const XML_Char *name)
{
	struct stream_callback_data *data = userData;
	--(data->depth);
	trim_whitespace_from_string(data->value);
	const char *tag;
	size_t handler_index;
	const char *sep_pos = strchr(name, XML_NAMESPACE_SEPARATOR);
	if (sep_pos != NULL) {
		tag = sep_pos + 1;
		handler_index = get_namespace_handler_index_by_namespace_name(name, sep_pos - name);
	} else {
		tag = name;
		handler_index = data->xml_format;
	}
	INFO("Element close: %s (%s)", tag, name);
	if (handler_index != XML_FORMATS_COUNT) {
		const struct xml_element_handler *handlers = namespace_handlers[handler_index].handlers;
		for (size_t i = 0; handlers[i].name != NULL; ++i) {
			if (strcmp(tag, handlers[i].name) == 0) {
				if (handlers[i].bitpos != 0) {
					if ((data->xml_pos[handler_index] & handlers[i].bitpos) == 0) {
						return;
					}
					data->xml_pos[handler_index] &= ~handlers[i].bitpos;
				}
				if (handlers[i].end_handle != NULL) {
					handlers[i].end_handle(data);
				}
				return;
			}
		}
	}
}

static void
character_data_handler(void *userData, const XML_Char *s, int len)
{
	struct stream_callback_data *data = userData;
	catas(data->value, s, len);
}

bool
engage_xml_parser(struct stream_callback_data *data)
{
	data->value = crtes();
	if (data->value == NULL) {
		return false;
	}
	data->xml_parser = XML_ParserCreateNS(NULL, XML_NAMESPACE_SEPARATOR);
	if (data->xml_parser == NULL) {
		free_string(data->value);
		return false;
	}
	data->xml_format = XML_FORMATS_COUNT;
	XML_SetUserData(data->xml_parser, data);
	XML_SetElementHandler(data->xml_parser, start_element_handler, end_element_handler);
	XML_SetCharacterDataHandler(data->xml_parser, character_data_handler);
	return true;
}

void
free_xml_parser(struct stream_callback_data *data)
{
	free_string(data->value);
	XML_Parse(data->xml_parser, NULL, 0, true); // final call
	XML_ParserFree(data->xml_parser);
}
