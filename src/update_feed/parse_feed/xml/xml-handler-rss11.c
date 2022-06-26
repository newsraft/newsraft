#include "update_feed/parse_feed/xml/parse_xml_feed.h"

// https://web.archive.org/web/20210411040907/http://inamidst.com/rss1.1/

static int8_t
link_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (crtss_or_cpyss(&data->feed.item->url, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == GENERIC_FEED) {
		if (crtss_or_cpyss(&data->feed.url, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_rss11_handlers[] = {
	{"item",        GENERIC_ITEM,    &generic_item_starter, NULL},
	{"title",       XML_UNKNOWN_POS, NULL,                  &generic_title_end},
	{"link",        XML_UNKNOWN_POS, NULL,                  &link_end},
	{"description", XML_UNKNOWN_POS, NULL,                  &generic_plain_content_end},
	//{"image",       RSS11_IMAGE,     NULL,                  &image_end},
	//{"url",         XML_UNKNOWN_POS, NULL,                  &url_end},
	{"Channel",     GENERIC_FEED,    NULL,                  NULL},
	{NULL,          XML_UNKNOWN_POS, NULL,                  NULL},
};
