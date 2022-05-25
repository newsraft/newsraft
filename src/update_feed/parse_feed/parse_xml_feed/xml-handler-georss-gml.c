#ifdef FEEDEATER_FORMAT_SUPPORT_GEORSS_GML
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// https://en.wikipedia.org/wiki/Geography_Markup_Language
// https://georss.org/gml.html

static void
pos_end(struct xml_data *data, const TidyAttr attrs)
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

const struct xml_element_handler xml_georss_gml_handlers[] = {
	{"pos", GEORSS_GML_POS,  NULL, &pos_end},
	{NULL,  GEORSS_GML_NONE, NULL, NULL},
};
#endif // FEEDEATER_FORMAT_SUPPORT_GEORSS_GML
