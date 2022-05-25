#ifdef FEEDEATER_FORMAT_SUPPORT_GEORSS
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// https://georss.org

static void
point_end(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if (we_are_inside_item(data) == false) {
		return;
	}
	if (prepend_empty_string_to_string_list(&data->feed->item->location) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (cpyss(data->feed->item->location->str, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

const struct xml_element_handler xml_georss_handlers[] = {
	{"point", GEORSS_POINT, NULL, &point_end},
	{NULL,    GEORSS_NONE,  NULL, NULL},
};
#endif // FEEDEATER_FORMAT_SUPPORT_GEORSS
