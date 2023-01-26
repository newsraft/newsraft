#include <string.h>
#include "update_feed/parse_xml/parse_xml_feed.h"

#define XML_NAMESPACE_SEPARATOR ' '

enum xml_format_index {
#ifndef NEWSRAFT_DISABLE_FORMAT_ATOM10
	ATOM10_FORMAT = 0,
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_RSS
	RSS20_FORMAT,
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_RSSCONTENT
	RSSCONTENT_FORMAT,
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_DUBLINCORE
	DUBLINCORE_FORMAT,
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_MEDIARSS
	MEDIARSS_FORMAT,
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_XHTML
	XHTML_FORMAT,
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_GEORSS
	GEORSS_FORMAT,
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_GEORSS_GML
	GEORSS_GML_FORMAT,
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_RSS
	RSS11_FORMAT,
	RSS10_FORMAT,
	RSS09_FORMAT,
#endif
	XML_FORMATS_COUNT,
};

struct namespace_handler {
	const char *const uri;
	const size_t len;
	const struct xml_element_handler *const handlers;
};

static const struct namespace_handler namespace_handlers[] = {
#ifndef NEWSRAFT_DISABLE_FORMAT_ATOM10
	{"http://www.w3.org/2005/Atom", 27, xml_atom10_handlers},
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_RSS
	{"http://backend.userland.com/rss2", 32, xml_rss_handlers},
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_RSSCONTENT
	{"http://purl.org/rss/1.0/modules/content/", 40, xml_rsscontent_handlers},
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_DUBLINCORE
	{"http://purl.org/dc/elements/1.1/", 32, xml_dublincore_handlers},
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_MEDIARSS
	{"http://search.yahoo.com/mrss/", 29, xml_mediarss_handlers},
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_XHTML
	{"http://www.w3.org/1999/xhtml", 28, xml_xhtml_handlers},
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_GEORSS
	{"http://www.georss.org/georss", 28, xml_georss_handlers},
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_GEORSS_GML
	{"http://www.opengis.net/gml", 26, xml_georss_gml_handlers},
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_RSS
	{"http://purl.org/net/rss1.1#", 27, xml_rss_handlers},
	{"http://purl.org/rss/1.0/", 24, xml_rss_handlers},
	{"http://channel.netscape.com/rdf/simple/0.9/", 43, xml_rss_handlers},
#endif
	{NULL, 0, NULL}
};

static inline size_t
get_namespace_handler_index_by_namespace_name(const char *ns, size_t ns_len)
{
	for (size_t i = 0; namespace_handlers[i].uri != NULL; ++i) {
		if ((ns_len == namespace_handlers[i].len) && (memcmp(ns, namespace_handlers[i].uri, ns_len) == 0)) {
			return i;
		}
	}
	return XML_FORMATS_COUNT;
}

static void
start_element_handler(void *userData, const XML_Char *name, const XML_Char **atts)
{
	struct stream_callback_data *data = userData;
	empty_string(data->emptying_target);
	data->depth += 1;
	data->path[data->depth] = XML_UNKNOWN_POS;
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
		size_t i = 0;
		for (; handlers[i].name != NULL; ++i) {
			if (strcmp(tag, handlers[i].name) == 0) {
				data->path[data->depth] = handlers[i].bitpos;
				if (handlers[i].start_handle != NULL) {
					data->depth -= 1;
					handlers[i].start_handle(data, atts);
					data->depth += 1;
				}
				return;
			}
		}
		if (handlers[i].start_handle != NULL) {
			handlers[i].start_handle(data, atts); // Default handler.
			return;
		}
	}
	if (data->xml_format != XML_FORMATS_COUNT) {
		return;
	}
#ifndef NEWSRAFT_DISABLE_FORMAT_RSS
	if ((data->depth == 1) && (strcmp(name, "rss") == 0)) {
		// Note to the future.
		// Since all versions of RSS specifications are basically compatible
		// with each other, we don't even have to check for a value in the
		// version attribute of the rss element.
		// https://www.rssboard.org/rss-specification
		// http://inamidst.com/rss1.1/
		// https://web.resource.org/rss/1.0/spec
		// http://backend.userland.com/rss093
		// https://www.rssboard.org/rss-0-9-2
		// http://static.userland.com/gems/backend/gratefulDead.xml
		// https://www.rssboard.org/rss-0-9-1-netscape
		// https://www.rssboard.org/rss-0-9-0
		// http://backend.userland.com/rssChangeNotes
		data->xml_format = RSS20_FORMAT;
	}
#endif
}

static void
end_element_handler(void *userData, const XML_Char *name)
{
	struct stream_callback_data *data = userData;
	trim_whitespace_from_string(data->text);
	if (data->depth > 0) {
		data->depth -= 1;
	}
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
		size_t i = 0;
		for (; handlers[i].name != NULL; ++i) {
			if (strcmp(tag, handlers[i].name) == 0) {
				if (handlers[i].end_handle != NULL) {
					handlers[i].end_handle(data);
				}
				return;
			}
		}
		if (handlers[i].end_handle != NULL) {
			handlers[i].end_handle(data); // Default handler.
			return;
		}
	}
}

static void
character_data_handler(void *userData, const XML_Char *s, int len)
{
	struct stream_callback_data *data = userData;
	catas(data->text, s, len);
}

static void
xml_default_handler(void *userData, const XML_Char *s, int len)
{
	struct stream_callback_data *data = userData;
	if (data->emptying_target == &data->decoy) {
		catas(data->text, s, len);
	}
}

bool
engage_xml_parser(struct stream_callback_data *data)
{
	data->text = crtes(50000);
	if (data->text == NULL) {
		return false;
	}
	data->xml_parser = XML_ParserCreateNS(NULL, XML_NAMESPACE_SEPARATOR);
	if (data->xml_parser == NULL) {
		free_string(data->text);
		return false;
	}
	static char ptr_for_decoy[1];
	data->decoy.ptr = ptr_for_decoy;
	data->xml_format = XML_FORMATS_COUNT;
	data->emptying_target = data->text;
	XML_SetUserData(data->xml_parser, data);
	XML_SetElementHandler(data->xml_parser, &start_element_handler, &end_element_handler);
	XML_SetCharacterDataHandler(data->xml_parser, &character_data_handler);
	XML_SetDefaultHandler(data->xml_parser, &xml_default_handler);
	return true;
}

void
free_xml_parser(struct stream_callback_data *data)
{
	XML_Parse(data->xml_parser, NULL, 0, true); // Final parsing call.
	XML_ParserFree(data->xml_parser);
	free_string(data->text);
}
