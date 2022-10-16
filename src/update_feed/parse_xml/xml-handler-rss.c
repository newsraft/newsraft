#include <string.h>
#include "update_feed/parse_xml/parse_xml_feed.h"

// https://web.archive.org/web/20211208135333/https://validator.w3.org/feed/docs/rss2.html

// Some ancient stuff:
// https://web.archive.org/web/20211011074123/https://www.rssboard.org/rss-0-9-0
// https://web.archive.org/web/20211106023928/https://web.resource.org/rss/1.0/spec
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

static int8_t
pub_date_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		data->feed.item->publication_date = parse_date_rfc822(data->text);
	} else if (data->path[data->depth] == GENERIC_FEED) {
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
	if (data->path[data->depth] == GENERIC_FEED) {
		data->feed.update_date = parse_date_rfc822(data->text);
	}
	return PARSE_OKAY;
}

static int8_t
author_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (serialize_caret(&data->feed.item->persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.item->persons, "type", 4, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.item->persons, "email", 5, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == GENERIC_FEED) {
		if (serialize_caret(&data->feed.persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.persons, "type", 4, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.persons, "email", 5, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
enclosure_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (data->path[data->depth] != GENERIC_ITEM) {
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
	if (serialize_caret(&data->feed.item->attachments) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_array(&data->feed.item->attachments, "url", 3, attr, attr_len) == false) {
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
comments_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (serialize_caret(&data->feed.item->attachments) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.item->attachments, "url", 3, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.item->attachments, "content", 7, "comments", 8) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
ttl_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == GENERIC_FEED) {
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
	if (data->path[data->depth] == GENERIC_FEED) {
		if (serialize_caret(&data->feed.persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.persons, "type", 4, "webmaster", 9) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.persons, "email", 5, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
managing_editor_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == GENERIC_FEED) {
		if (serialize_caret(&data->feed.persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.persons, "type", 4, "editor", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.persons, "email", 5, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
source_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (data->path[data->depth] != GENERIC_ITEM) {
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
	if (serialize_caret(&data->feed.item->attachments) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_array(&data->feed.item->attachments, "url", 3, attr, attr_len) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_array(&data->feed.item->attachments, "content", 7, "source", 6) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

static int8_t
source_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (serialize_string(&data->feed.item->attachments, "title", 5, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_rss_handlers[] = {
	{"item",           GENERIC_ITEM,    &generic_item_starter, &generic_item_ender},
	{"guid",           XML_UNKNOWN_POS, NULL,                  &generic_guid_end},
	{"title",          XML_UNKNOWN_POS, NULL,                  &generic_title_end},
	{"link",           XML_UNKNOWN_POS, NULL,                  &link_end},
	{"description",    XML_UNKNOWN_POS, NULL,                  &generic_html_content_end},
	{"pubDate",        XML_UNKNOWN_POS, NULL,                  &pub_date_end},
	{"lastBuildDate",  XML_UNKNOWN_POS, NULL,                  &last_build_date_end},
	{"author",         XML_UNKNOWN_POS, NULL,                  &author_end},
	{"enclosure",      XML_UNKNOWN_POS, &enclosure_start,      NULL},
	{"category",       XML_UNKNOWN_POS, NULL,                  &generic_category_end},
	{"comments",       XML_UNKNOWN_POS, NULL,                  &comments_end},
	{"ttl",            XML_UNKNOWN_POS, NULL,                  &ttl_end},
	{"generator",      XML_UNKNOWN_POS, NULL,                  &generator_end},
	{"webMaster",      XML_UNKNOWN_POS, NULL,                  &web_master_end},
	{"managingEditor", XML_UNKNOWN_POS, NULL,                  &managing_editor_end},
	{"source",         XML_UNKNOWN_POS, &source_start,         &source_end},
	{"channel",        GENERIC_FEED,    NULL,                  NULL},
	// Channel with capital C is used in RSS 1.1
	{"Channel",        GENERIC_FEED,    NULL,                  NULL},
	{NULL,             XML_UNKNOWN_POS, NULL,                  NULL},
};
