#include <stdio.h>
#include <string.h>
#include "update_feed/parse_feed/xml/parse_xml_feed.h"

// https://web.archive.org/web/20211208135333/https://validator.w3.org/feed/docs/rss2.html

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
guid_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		if (crtss_or_cpyss(&data->feed.item->guid, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
title_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		if (crtss_or_cpyss(&data->feed.item->title.value, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (crtas_or_cpyas(&data->feed.item->title.type, "text/plain", 10) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == RSS20_CHANNEL) {
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
	if (data->path[data->depth] == RSS20_ITEM) {
		if (crtss_or_cpyss(&data->feed.item->url, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == RSS20_CHANNEL) {
		if (crtss_or_cpyss(&data->feed.url, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
description_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		if (cat_caret_to_serialization(&data->feed.item->content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_array_to_serialization(&data->feed.item->content, "type", 4, "text/html", 9) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.item->content, "text", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == RSS20_CHANNEL) {
		if (cat_caret_to_serialization(&data->feed.content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_array_to_serialization(&data->feed.content, "type", 4, "text/html", 9) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.content, "text", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
pub_date_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		data->feed.item->pubdate = parse_date_rfc822(data->text);
	} else if (data->path[data->depth] == RSS20_CHANNEL) {
		// Some RSS 2.0 feeds use lastBuildDate and some
		// use pubDate for showing last update time of channel.
		// But lastBuildDate is more commonly used, so don't
		// bother with pubDate value if lastBuildDate was already
		// set.
		if (data->feed.update_date == 0) {
			data->feed.update_date = parse_date_rfc822(data->text);
		}
	}
	return PARSE_OKAY;
}

static int8_t
last_build_date_end(struct stream_callback_data *data)
{
	// In RSS 2.0 lastBuildDate element is only for channel,
	// for items they use pubDate.
	if (data->path[data->depth] == RSS20_CHANNEL) {
		data->feed.update_date = parse_date_rfc822(data->text);
	}
	return PARSE_OKAY;
}

static int8_t
author_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		if (cat_caret_to_serialization(&data->feed.item->persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_array_to_serialization(&data->feed.item->persons, "type", 4, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.item->persons, "email", 5, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == RSS20_CHANNEL) {
		if (cat_caret_to_serialization(&data->feed.persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_array_to_serialization(&data->feed.persons, "type", 4, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.persons, "email", 5, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
enclosure_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (data->path[data->depth] != RSS20_ITEM) {
		return PARSE_OKAY;
	}
	const char *attr = get_value_of_attribute_key(attrs, "url");
	if (attr == NULL) {
		return PARSE_OKAY;
	}
	const size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return PARSE_OKAY;
	}
	if (cat_caret_to_serialization(&data->feed.item->attachments) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (cat_array_to_serialization(&data->feed.item->attachments, "url", 3, attr, attr_len) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_attribute(&data->feed.item->attachments, attrs, "type", "type", 4) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_attribute(&data->feed.item->attachments, attrs, "length", "size", 4) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

static int8_t
category_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		if (cat_caret_to_serialization(&data->feed.item->categories) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.item->categories, "term", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == RSS20_CHANNEL) {
		if (cat_caret_to_serialization(&data->feed.categories) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.categories, "term", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
comments_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		if (cat_caret_to_serialization(&data->feed.item->attachments) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.item->attachments, "url", 3, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_array_to_serialization(&data->feed.item->attachments, "content", 7, "comments", 8) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
ttl_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_CHANNEL) {
		int64_t minutes;
		if (sscanf(data->text->ptr, "%" SCNd64, &minutes) != 1) {
			WARN("Couldn't convert value of <ttl> element to a number!");
			return PARSE_OKAY; // Continue parsing like nothing happened.
		}
		if (minutes < 0) {
			WARN("Value of <ttl> element is negative!");
			return PARSE_OKAY; // Continue parsing like nothing happened.
		}
		data->feed.time_to_live = minutes * 60;
	}
	return PARSE_OKAY;
}

static int8_t
web_master_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_CHANNEL) {
		if (cat_caret_to_serialization(&data->feed.persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_array_to_serialization(&data->feed.persons, "type", 4, "webmaster", 9) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.persons, "email", 5, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
managing_editor_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_CHANNEL) {
		if (cat_caret_to_serialization(&data->feed.persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_array_to_serialization(&data->feed.persons, "type", 4, "editor", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.persons, "email", 5, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
source_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (data->path[data->depth] != RSS20_ITEM) {
		return PARSE_OKAY;
	}
	const char *attr = get_value_of_attribute_key(attrs, "url");
	if (attr == NULL) {
		return PARSE_OKAY;
	}
	const size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return PARSE_OKAY;
	}
	if (cat_caret_to_serialization(&data->feed.item->attachments) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (cat_array_to_serialization(&data->feed.item->attachments, "url", 3, attr, attr_len) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (cat_array_to_serialization(&data->feed.item->attachments, "content", 7, "source", 6) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

static int8_t
source_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == RSS20_ITEM) {
		if (cat_string_to_serialization(&data->feed.item->attachments, "title", 5, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_rss20_handlers[] = {
	{"item",           RSS20_ITEM,           &item_start,      NULL},
	{"guid",           RSS20_GUID,           NULL,             &guid_end},
	{"title",          RSS20_TITLE,          NULL,             &title_end},
	{"link",           RSS20_LINK,           NULL,             &link_end},
	{"description",    RSS20_DESCRIPTION,    NULL,             &description_end},
	{"pubDate",        RSS20_PUBDATE,        NULL,             &pub_date_end},
	{"lastBuildDate",  RSS20_LASTBUILDDATE,  NULL,             &last_build_date_end},
	{"author",         RSS20_AUTHOR,         NULL,             &author_end},
	{"enclosure",      XML_UNKNOWN_POS,      &enclosure_start, NULL},
	{"category",       RSS20_CATEGORY,       NULL,             &category_end},
	{"comments",       RSS20_COMMENTS,       NULL,             &comments_end},
	{"ttl",            RSS20_TTL,            NULL,             &ttl_end},
	{"generator",      RSS20_GENERATOR,      NULL,             &generator_end},
	{"webMaster",      RSS20_WEBMASTER,      NULL,             &web_master_end},
	{"managingEditor", RSS20_MANAGINGEDITOR, NULL,             &managing_editor_end},
	{"source",         RSS20_SOURCE,         &source_start,    &source_end},
	{"channel",        RSS20_CHANNEL,        NULL,             NULL},
	{NULL,             XML_UNKNOWN_POS,      NULL,             NULL},
};
