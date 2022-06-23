#include "update_feed/parse_feed/xml/parse_xml_feed.h"

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
		if (crtss_or_cpyss(&data->feed.item->title, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == RSS11_CHANNEL) {
		if (crtss_or_cpyss(&data->feed.title, data->text) == false) {
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
	// Specification explicitly says that this is a plain text.
	if (data->path[data->depth] == RSS11_ITEM) {
		if (cat_caret_to_serialization(&data->feed.item->content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.item->content, "text", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == RSS11_CHANNEL) {
		if (cat_caret_to_serialization(&data->feed.content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.content, "text", 4, data->text) == false) {
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
