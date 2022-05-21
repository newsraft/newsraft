#ifdef FEEDEATER_FORMAT_SUPPORT_RSS11
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// Despite this file named "xml-handler-rss11.c", it is actually
// considered to parse RSS 0.9, RSS 1.0 and RSS 1.1 feeds.
//
// Some useful notes:
//
// All of these formats do support image attachments with the <image> tag
// but the flaw is that you can't include image elements in items.
// They are only for channel overall...
//
// https://web.archive.org/web/20211011074123/https://www.rssboard.org/rss-0-9-0
// https://web.archive.org/web/20211106023928/https://web.resource.org/rss/1.0/spec
// https://web.archive.org/web/20210411040907/http://inamidst.com/rss1.1/

static void
item_start(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	prepend_item(&data->feed->item);
}

static void
title_end(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if ((data->xml_pos[RSS11_FORMAT] & RSS11_ITEM) != 0) {
		if (cpyss(data->feed->item->title.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed->title.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
link_end(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if ((data->xml_pos[RSS11_FORMAT] & RSS11_ITEM) != 0) {
		if (cpyss(data->feed->item->url, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed->url, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

static void
description_end(struct xml_data *data, const TidyAttr attrs)
{
	(void)attrs;
	if ((data->xml_pos[RSS11_FORMAT] & RSS11_ITEM) != 0) {
		if (cpyss(data->feed->item->content.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else {
		if (cpyss(data->feed->summary.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

const struct xml_element_handler xml_rss11_handlers[] = {
	{"item",        RSS11_ITEM,        &item_start, NULL},
	{"title",       RSS11_TITLE,       NULL,        &title_end},
	{"link",        RSS11_LINK,        NULL,        &link_end},
	{"description", RSS11_DESCRIPTION, NULL,        &description_end},
	//{"image",       RSS11_IMAGE,       NULL,        &image_end},
	//{"url",         RSS11_URL,         NULL,        &url_end},
	{NULL,          RSS11_NONE,        NULL,        NULL},
};
#endif // FEEDEATER_FORMAT_SUPPORT_RSS11
