#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS11
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// Some useful notes:
//
// All of these formats do support image attachments with the <image> tag
// but the flaw is that you can't include image elements in items.
// They are only for channel overall...
//
// https://web.archive.org/web/20210411040907/http://inamidst.com/rss1.1/

static int8_t
item_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)attrs;
	if (prepend_item(&data->feed.item) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

static int8_t
title_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS11_ITEM) {
		if (crtss_or_cpyss(&data->feed.item->title.value, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (crtas_or_cpyas(&data->feed.item->title.type, "text/plain", 10) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == RSS11_CHANNEL) {
		if (crtss_or_cpyss(&data->feed.title.value, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (crtas_or_cpyas(&data->feed.title.type, "text/plain", 10) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
link_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS11_ITEM) {
		if (crtss_or_cpyss(&data->feed.item->url, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == RSS11_CHANNEL) {
		if (crtss_or_cpyss(&data->feed.url, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
description_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS11_ITEM) {
		if (crtss_or_cpyss(&data->feed.item->content.value, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		// Specification explicitly says that this is plain text.
		if (crtas_or_cpyas(&data->feed.item->content.type, "text/plain", 10) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == RSS11_CHANNEL) {
		if (crtss_or_cpyss(&data->feed.summary.value, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		// Specification explicitly says that this is plain text.
		if (crtas_or_cpyas(&data->feed.summary.type, "text/plain", 10) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_rss11_handlers[] = {
	{"item",        RSS11_ITEM,        &item_start, NULL},
	{"title",       RSS11_TITLE,       NULL,        &title_end},
	{"link",        RSS11_LINK,        NULL,        &link_end},
	{"description", RSS11_DESCRIPTION, NULL,        &description_end},
	//{"image",       RSS11_IMAGE,       NULL,        &image_end},
	//{"url",         RSS11_URL,         NULL,        &url_end},
	{"Channel",     RSS11_CHANNEL,     NULL,        NULL},
	{NULL,          XML_UNKNOWN_POS,   NULL,        NULL},
};
#endif // NEWSRAFT_FORMAT_SUPPORT_RSS11
