#ifdef NEWSRAFT_FORMAT_SUPPORT_RSSCONTENT
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// https://web.archive.org/web/20211201074403/https://web.resource.org/rss/1.0/modules/content/

static int8_t
encoded_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == false) {
		return PARSE_OKAY;
	}
	if ((data->feed.item->content.value != NULL) && (data->value->len < data->feed.item->content.value->len)) {
		return PARSE_OKAY;
	}
	if (crtss_or_cpyss(&data->feed.item->content.value, data->value) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (crtas_or_cpyas(&data->feed.item->content.type, "text/html", 9) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_rsscontent_handlers[] = {
	{"encoded", RSSCONTENT_ENCODED, NULL, &encoded_end},
	{NULL,      XML_UNKNOWN_POS,    NULL, NULL},
};
#endif // NEWSRAFT_FORMAT_SUPPORT_RSSCONTENT
