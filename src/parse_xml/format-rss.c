#include <string.h>
#include "parse_xml/parse_xml_feed.h"

// Note to the future.
//
// Since all versions of RSS specifications are basically compatible
// with each other, we don't even have to check for a value in the
// version attribute of the rss element.
//
// https://web.archive.org/web/20240202013527/http://backend.userland.com/rssChangeNotes
// https://web.archive.org/web/20230124122614/http://static.userland.com/gems/backend/gratefulDead.xml
// https://web.archive.org/web/20211011074123/https://www.rssboard.org/rss-0-9-0
// https://web.archive.org/web/20231208183902/https://www.rssboard.org/rss-0-9-1-netscape
// https://web.archive.org/web/20231216194351/https://www.rssboard.org/rss-0-9-2
// https://web.archive.org/web/20240202013313/http://backend.userland.com/rss093
// https://web.archive.org/web/20060831192123/http://web.resource.org/rss/1.0/spec
// https://web.archive.org/web/20210411040907/http://inamidst.com/rss1.1/
// https://web.archive.org/web/20211208135333/https://validator.w3.org/feed/docs/rss2.html
// https://web.archive.org/web/20240201121905/https://www.rssboard.org/rss-specification

static int8_t
rss_guid_start(struct feed_update_state *data, const XML_Char **attrs)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		const char *val = get_value_of_attribute_key(attrs, "isPermaLink");
		// Default value of isPermaLink is considered true!
		data->feed.item->guid_is_url = val == NULL || strcmp(val, "true") == 0;
	}
	return PARSE_OKAY;
}

static int8_t
rss_link_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (cpyss(&data->feed.item->url, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == GENERIC_FEED) {
		if (cpyss(&data->feed.url, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
rss_pubdate_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		data->feed.item->publication_date = parse_date(data->text->ptr, false);
		if (data->feed.item->publication_date < 0) {
			data->feed.item->publication_date = 0;
		}
	}
	return PARSE_OKAY;
}

static int8_t
rss_author_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (serialize_caret(&data->feed.item->persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.item->persons, "type=", 5, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.item->persons, "email=", 6, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == GENERIC_FEED) {
		if (serialize_caret(&data->feed.persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.persons, "type=", 5, "author", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.persons, "email=", 6, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
rss_enclosure_start(struct feed_update_state *data, const XML_Char **attrs)
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
	if (serialize_array(&data->feed.item->attachments, "url=", 4, attr, attr_len) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_attribute(&data->feed.item->attachments, "type=", 5, attrs, "type") == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_attribute(&data->feed.item->attachments, "size=", 5, attrs, "length") == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

static int8_t
rss_comments_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (serialize_caret(&data->feed.item->attachments) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.item->attachments, "url=", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.item->attachments, "content=", 8, "comments", 8) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
rss_ttl_end(struct feed_update_state *data)
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
rss_managingeditor_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_FEED) {
		if (serialize_caret(&data->feed.persons) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.persons, "type=", 5, "editor", 6) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.persons, "email=", 6, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
rss_source_start(struct feed_update_state *data, const XML_Char **attrs)
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
	if (serialize_array(&data->feed.item->attachments, "url=", 4, attr, attr_len) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_array(&data->feed.item->attachments, "content=", 8, "source", 6) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

static int8_t
rss_source_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (serialize_string(&data->feed.item->attachments, "title=", 6, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}
