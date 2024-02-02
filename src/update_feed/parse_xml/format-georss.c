#include "update_feed/parse_xml/parse_xml_feed.h"

// https://georss.org
// https://georss.org/gml.html
// https://en.wikipedia.org/wiki/Geography_Markup_Language

static int8_t
georss_point_end(struct stream_callback_data *data)
{
	if (data->in_item == true) {
		if (serialize_caret(&data->feed.item->extras) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.item->extras, "coordinates=", 12, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}
