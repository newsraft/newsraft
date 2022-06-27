#include "update_feed/parse_feed/xml/parse_xml_feed.h"

// https://en.wikipedia.org/wiki/Geography_Markup_Language
// https://georss.org/gml.html

static int8_t
pos_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if (serialize_caret(&data->feed.item->extras) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.item->extras, "coordinates", 11, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_georss_gml_handlers[] = {
	{"pos", XML_UNKNOWN_POS, NULL, &pos_end},
	{NULL,  XML_UNKNOWN_POS, NULL, NULL},
};
