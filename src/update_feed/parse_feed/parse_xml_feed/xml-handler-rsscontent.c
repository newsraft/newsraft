#ifdef FEEDEATER_FORMAT_SUPPORT_RSSCONTENT
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// https://web.archive.org/web/20211201074403/https://web.resource.org/rss/1.0/modules/content/

static void
encoded_end(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if (we_are_inside_item(data) == false) {
		return;
	}
	if (data->feed->item->content.value->len > data->value->len) {
		return;
	}
	if (cpyss(data->feed->item->content.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (cpyas(data->feed->item->content.type, "text/html", 9) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

const struct xml_element_handler xml_rsscontent_handlers[] = {
	{"encoded", RSSCONTENT_ENCODED, NULL, &encoded_end},
	{NULL,      RSSCONTENT_NONE,    NULL, NULL},
};
#endif // FEEDEATER_FORMAT_SUPPORT_RSSCONTENT
