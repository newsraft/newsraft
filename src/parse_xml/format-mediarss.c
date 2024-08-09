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

static int8_t
mediarss_content_start(struct feed_update_state *data, const XML_Char **attrs)
{
	const char *attr = get_value_of_attribute_key(attrs, "url");
	if (attr == NULL) {
		return PARSE_OKAY; // Ignore empty content entries.
	}
	const size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return PARSE_OKAY; // Ignore empty content entries.
	}
	struct string **dest = data->in_item ? &data->feed.item->attachments : &data->feed.attachments;
	if (serialize_caret(dest) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_array(dest, "url=", 4, attr, attr_len) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_attribute(dest, "type=", 5, attrs, "type") == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_attribute(dest, "size=", 5, attrs, "fileSize") == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_attribute(dest, "duration=", 9, attrs, "duration") == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_attribute(dest, "width=", 6, attrs, "width") == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_attribute(dest, "height=", 7, attrs, "height") == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

static int8_t
embed_or_player_start(struct feed_update_state *data, const XML_Char **attrs)
{
	const char *attr = get_value_of_attribute_key(attrs, "url");
	if (attr == NULL) {
		return PARSE_OKAY; // Ignore empty entries.
	}
	const size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return PARSE_OKAY; // Ignore empty entries.
	}
	struct string **dest = data->in_item ? &data->feed.item->attachments : &data->feed.attachments;
	if (serialize_caret(dest) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_array(dest, "url=", 4, attr, attr_len) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

static int8_t
peerlink_start(struct feed_update_state *data, const XML_Char **attrs)
{
	const char *attr = get_value_of_attribute_key(attrs, "href");
	if (attr == NULL) {
		return PARSE_OKAY; // Ignore empty entries.
	}
	const size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return PARSE_OKAY; // Ignore empty entries.
	}
	struct string **dest = data->in_item ? &data->feed.item->attachments : &data->feed.attachments;
	if (serialize_caret(dest) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_array(dest, "url=", 4, attr, attr_len) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_attribute(dest, "type=", 5, attrs, "type") == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

static int8_t
description_start(struct feed_update_state *data, const XML_Char **attrs)
{
	if (data->in_item == true) {
		if (data->path[data->depth] == MEDIARSS_CONTENT) {
			if (serialize_attribute(&data->feed.item->attachments, "description_type=", 17, attrs, "type") == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else {
			if (serialize_caret(&data->feed.item->content) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
			if (serialize_attribute(&data->feed.item->content, "type=", 5, attrs, "type") == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
description_end(struct feed_update_state *data)
{
	if (data->in_item == true) {
		if (data->path[data->depth] == MEDIARSS_CONTENT) {
			if (serialize_string(&data->feed.item->attachments, "description_text=", 17, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else {
			if (serialize_string(&data->feed.item->content, "text=", 5, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}
