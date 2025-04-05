#include "parse_xml/parse_xml_feed.h"

// https://georss.org
// https://georss.org/gml.html
// https://en.wikipedia.org/wiki/Geography_Markup_Language

static int8_t
georss_point_end(struct feed_update_state *data)
{
	if (data->in_item == true) {
		serialize_caret(&data->feed.item->extras);
		serialize_string(&data->feed.item->extras, "coordinates=", 12, data->text);
	}
	return PARSE_OKAY;
}
