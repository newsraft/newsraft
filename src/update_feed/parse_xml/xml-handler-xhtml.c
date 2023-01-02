#include "update_feed/parse_xml/parse_xml_feed.h"

static int8_t
xhtml_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)attrs;
	XML_DefaultCurrent(data->xml_parser);
	return PARSE_OKAY;
}

static int8_t
xhtml_end(struct stream_callback_data *data)
{
	XML_DefaultCurrent(data->xml_parser);
	return PARSE_OKAY;
}

const struct xml_element_handler xml_xhtml_handlers[] = {
	{NULL, XML_UNKNOWN_POS, &xhtml_start, &xhtml_end},
};
