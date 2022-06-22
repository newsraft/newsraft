#include "update_feed/parse_feed/xml/parse_xml_feed.h"

// https://web.resource.org/rss/1.0/modules/content/

static int8_t
encoded_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if (cat_caret_to_serialization(&data->feed.item->content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_array_to_serialization(&data->feed.item->content, "type", 4, "text/html", 9) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.item->content, "text", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_rsscontent_handlers[] = {
	{"encoded", RSSCONTENT_ENCODED, NULL, &encoded_end},
	{NULL,      XML_UNKNOWN_POS,    NULL, NULL},
};
