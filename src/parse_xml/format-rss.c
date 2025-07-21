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

static void
rss_guid_start(struct feed_update_state *data, const XML_Char **attrs)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		const char *val = get_value_of_attribute_key(attrs, "isPermaLink");
		// Default value of isPermaLink is considered true!
		data->feed.item->guid_is_url = val == NULL || strcmp(val, "true") == 0;
	}
}

static void
rss_link_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		cpyss(&data->feed.item->url, data->text);
	} else if (data->path[data->depth] == GENERIC_FEED) {
		cpyss(&data->feed.link, data->text);
	}
}

static void
rss_pubdate_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		data->feed.item->publication_date = parse_date(data->text->ptr, false);
		if (data->feed.item->publication_date < 0) {
			data->feed.item->publication_date = 0;
		}
	}
}

static void
rss_author_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		serialize_caret(&data->feed.item->persons);
		serialize_array(&data->feed.item->persons, "type=", 5, "author", 6);
		serialize_string(&data->feed.item->persons, "email=", 6, data->text);
	} else if (data->path[data->depth] == GENERIC_FEED) {
		serialize_caret(&data->feed.persons);
		serialize_array(&data->feed.persons, "type=", 5, "author", 6);
		serialize_string(&data->feed.persons, "email=", 6, data->text);
	}
}

static void
rss_enclosure_start(struct feed_update_state *data, const XML_Char **attrs)
{
	if (data->path[data->depth] != GENERIC_ITEM) {
		return;
	}
	const char *attr = get_value_of_attribute_key(attrs, "url");
	if (attr == NULL) {
		return;
	}
	const size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return;
	}
	serialize_caret(&data->feed.item->attachments);
	serialize_array(&data->feed.item->attachments, "url=", 4, attr, attr_len);
	serialize_attribute(&data->feed.item->attachments, "type=", 5, attrs, "type");
	serialize_attribute(&data->feed.item->attachments, "size=", 5, attrs, "length");
}

static void
rss_comments_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		serialize_caret(&data->feed.item->attachments);
		serialize_string(&data->feed.item->attachments, "url=", 4, data->text);
		serialize_array(&data->feed.item->attachments, "content=", 8, "comments", 8);
	}
}

static void
rss_ttl_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_FEED) {
		int64_t minutes = -1;
		if (sscanf(data->text->ptr, "%" SCNd64, &minutes) != 1) {
			WARN("Couldn't convert value of <ttl> element to a number!");
			return; // Continue parsing like nothing happened.
		}
		if (minutes < 0) {
			WARN("Value of <ttl> element is negative!");
			return; // Continue parsing like nothing happened.
		}
		data->feed.time_to_live = minutes * 60;
	}
}

static void
rss_managingeditor_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_FEED) {
		serialize_caret(&data->feed.persons);
		serialize_array(&data->feed.persons, "type=", 5, "editor", 6);
		serialize_string(&data->feed.persons, "email=", 6, data->text);
	}
}

static void
rss_source_start(struct feed_update_state *data, const XML_Char **attrs)
{
	if (data->path[data->depth] != GENERIC_ITEM) {
		return;
	}
	const char *attr = get_value_of_attribute_key(attrs, "url");
	if (attr == NULL) {
		return;
	}
	const size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return;
	}
	serialize_caret(&data->feed.item->attachments);
	serialize_array(&data->feed.item->attachments, "url=", 4, attr, attr_len);
	serialize_array(&data->feed.item->attachments, "content=", 8, "source", 6);
}

static void
rss_source_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		serialize_string(&data->feed.item->attachments, "title=", 6, data->text);
	}
}
