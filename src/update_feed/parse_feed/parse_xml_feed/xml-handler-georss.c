#ifdef NEWSRAFT_FORMAT_SUPPORT_GEORSS
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// https://georss.org

static int8_t
point_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if (copy_string_to_string_list(&data->feed.item->location, data->value) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_georss_handlers[] = {
	{"point", GEORSS_POINT,    NULL, &point_end},
	{NULL,    XML_UNKNOWN_POS, NULL, NULL},
};
#endif // NEWSRAFT_FORMAT_SUPPORT_GEORSS
