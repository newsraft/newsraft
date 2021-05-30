#include "feedeater.h"

/* Generic parsing is based on processing namespaced tags.
 * Thus if tag does belong to some namespace and feedeater recognizes it,
 * it will be parsed by functions defined in namespaces directory. */

static void XMLCALL
process_element_start(void *userData, const XML_Char *name, const XML_Char **atts)
{
	struct feed_parser_data *data = userData;
	++(data->depth);
	process_namespaced_tag_start(userData, name, atts);
}

static void XMLCALL
process_element_end(void *userData, const XML_Char *name)
{
	struct feed_parser_data *data = userData;
	--(data->depth);
	value_strip_whitespace(data->value, &data->value_len);
	process_namespaced_tag_end(userData, name);
}

int
parse_generic(XML_Parser *parser, struct string *feed_url, struct feed_parser_data *feed_data)
{
	int error = 0;
	XML_SetElementHandler(*parser, &process_element_start, &process_element_end);
	XML_SetCharacterDataHandler(*parser, &store_xml_element_value);
	if (XML_ResumeParser(*parser) == XML_STATUS_ERROR) {
		error = 1;
		/*status_write("rss20: %" XML_FMT_STR " at line %" XML_FMT_INT_MOD "u\n",*/
              /*XML_ErrorString(XML_GetErrorCode(*parser)),*/
              /*XML_GetCurrentLineNumber(*parser));*/
	}
	return error;
}
