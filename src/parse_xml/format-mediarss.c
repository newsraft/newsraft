#include <string.h>
#include "parse_xml/parse_xml_feed.h"

// References:
// https://www.rssboard.org/media-rss
//
// Notes to the future.
//
// There is no need to parse thumbnail elements, because storing information
// about decorating pictures defeats the whole purpose of project being a
// console application with as few distractions as possible.
//
// Purpose of media:group element is to group content elements that are
// effectively the same content, but from the user's perspective, all
// attachments are just links with some metadata. Therefore, such grouping does
// not make sense and is ignored by the parser. Also, this grouping is difficult
// to reflect in the database and requires various sophistications.

static void
mediarss_content_start(struct feed_update_state *data, const XML_Char **attrs)
{
	const char *attr = get_value_of_attribute_key(attrs, "url");
	if (attr == NULL) {
		return; // Ignore empty content entries.
	}
	const size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return; // Ignore empty content entries.
	}
	struct string **dest = data->in_item ? &data->feed.item->attachments : &data->feed.attachments;
	serialize_caret(dest);
	serialize_array(dest, "url=", 4, attr, attr_len);
	serialize_attribute(dest, "type=", 5, attrs, "type");
	serialize_attribute(dest, "size=", 5, attrs, "fileSize");
	serialize_attribute(dest, "duration=", 9, attrs, "duration");
	serialize_attribute(dest, "width=", 6, attrs, "width");
	serialize_attribute(dest, "height=", 7, attrs, "height");
}

static void
embed_or_player_start(struct feed_update_state *data, const XML_Char **attrs)
{
	const char *attr = get_value_of_attribute_key(attrs, "url");
	if (attr == NULL) {
		return; // Ignore empty entries.
	}
	const size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return; // Ignore empty entries.
	}
	struct string **dest = data->in_item ? &data->feed.item->attachments : &data->feed.attachments;
	serialize_caret(dest);
	serialize_array(dest, "url=", 4, attr, attr_len);
}

static void
peerlink_start(struct feed_update_state *data, const XML_Char **attrs)
{
	const char *attr = get_value_of_attribute_key(attrs, "href");
	if (attr == NULL) {
		return; // Ignore empty entries.
	}
	const size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return; // Ignore empty entries.
	}
	struct string **dest = data->in_item ? &data->feed.item->attachments : &data->feed.attachments;
	serialize_caret(dest);
	serialize_array(dest, "url=", 4, attr, attr_len);
	serialize_attribute(dest, "type=", 5, attrs, "type");
}

static void
description_start(struct feed_update_state *data, const XML_Char **attrs)
{
	if (data->in_item) {
		if (data->path[data->depth] == MEDIARSS_CONTENT) {
			serialize_attribute(&data->feed.item->attachments, "description_type=", 17, attrs, "type");
		} else {
			serialize_caret(&data->feed.item->content);
			serialize_attribute(&data->feed.item->content, "type=", 5, attrs, "type");
		}
	}
}

static void
description_end(struct feed_update_state *data)
{
	if (data->in_item) {
		if (data->path[data->depth] == MEDIARSS_CONTENT) {
			serialize_string(&data->feed.item->attachments, "description_text=", 17, data->text);
		} else {
			serialize_string(&data->feed.item->content, "text=", 5, data->text);
		}
	}
}
