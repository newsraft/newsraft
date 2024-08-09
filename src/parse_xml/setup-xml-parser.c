#include <string.h>
#include "parse_xml/parse_xml_feed.h"

#define XML_NAMESPACE_SEPARATOR ' '

static void
start_element_handler(void *userData, const XML_Char *name, const XML_Char **atts)
{
	struct feed_update_state *data = userData;
	empty_string(data->emptying_target);
	data->depth += 1;
	data->path[data->depth] = XML_UNKNOWN_POS;
	struct xml_element_handler *handler = in_word_set(name, strlen(name));
	if (handler != NULL) {
		INFO("Known start: %s", name);
		data->path[data->depth] = handler->bitpos;
		if (handler->start_handle != NULL) {
			data->depth -= 1;
			handler->start_handle(data, atts);
			data->depth += 1;
		}
	} else if (strncmp(name, "http://www.w3.org/1999/xhtml", 28) == 0) {
		XML_DefaultCurrent(data->xml_parser);
	} else {
		WARN("Unknown start: %s", name);
	}
}

static void
end_element_handler(void *userData, const XML_Char *name)
{
	struct feed_update_state *data = userData;
	trim_whitespace_from_string(data->text);
	if (data->depth > 0) {
		data->depth -= 1;
	}
	struct xml_element_handler *handler = in_word_set(name, strlen(name));
	if (handler != NULL) {
		INFO("Known end: %s", name);
		if (handler->end_handle != NULL) {
			handler->end_handle(data);
		}
	} else if (strncmp(name, "http://www.w3.org/1999/xhtml", 28) == 0) {
		XML_DefaultCurrent(data->xml_parser);
	} else {
		WARN("Unknown end: %s", name);
	}
}

static void
character_data_handler(void *userData, const XML_Char *s, int len)
{
	catas(((struct feed_update_state *)userData)->text, s, len);
}

static void
xml_default_handler(void *userData, const XML_Char *s, int len)
{
	struct feed_update_state *data = userData;
	if (data->emptying_target == &data->decoy) {
		catas(data->text, s, len);
	}
}

bool
setup_xml_parser(struct feed_update_state *data)
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
	data->emptying_target = data->text;
	XML_SetUserData(data->xml_parser, data);
	XML_SetElementHandler(data->xml_parser, &start_element_handler, &end_element_handler);
	XML_SetCharacterDataHandler(data->xml_parser, &character_data_handler);
	XML_SetDefaultHandler(data->xml_parser, &xml_default_handler);
	return true;
}
