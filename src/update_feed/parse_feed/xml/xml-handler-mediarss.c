#include <string.h>
#include "update_feed/parse_feed/xml/parse_xml_feed.h"

// Useful links:
// https://www.rssboard.org/media-rss

static int8_t
group_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)attrs;
	if (we_are_inside_item(data) == true) {
		if (cat_caret_to_serialization(&data->feed.item->attachments) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
content_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (we_are_inside_item(data) == false) {
		return PARSE_OKAY; // Don't bother with global media elements.
	}
	const char *attr = get_value_of_attribute_key(attrs, "url");
	if (attr == NULL) {
		return PARSE_OKAY; // Ignore empty content entries.
	}
	const size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return PARSE_OKAY; // Ignore empty content entries.
	}
	if (data->path[data->depth] != MRSS_GROUP) {
		if (cat_caret_to_serialization(&data->feed.item->attachments) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	if (cat_array_to_serialization(&data->feed.item->attachments, "url", 3, attr, attr_len) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_attribute(&data->feed.item->attachments, attrs, "type", "type", 4) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_attribute(&data->feed.item->attachments, attrs, "fileSize", "size", 4) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_attribute(&data->feed.item->attachments, attrs, "duration", "duration", 8) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

static int8_t
thumbnail_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (we_are_inside_item(data) == false) {
		return PARSE_OKAY;
	}
	const char *attr = get_value_of_attribute_key(attrs, "url");
	if (attr == NULL) {
		return PARSE_OKAY; // Ignore empty thumbnail entries.
	}
	const size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return PARSE_OKAY; // Ignore empty thumbnail entries.
	}
	if (cat_caret_to_serialization(&data->feed.item->pictures) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (cat_array_to_serialization(&data->feed.item->pictures, "url", 3, attr, attr_len) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_attribute(&data->feed.item->pictures, attrs, "width", "width", 5) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	if (serialize_attribute(&data->feed.item->pictures, attrs, "height", "height", 6) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

static int8_t
description_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (we_are_inside_item(data) == true) {
		if (data->path[data->depth] == MRSS_GROUP) {
			if (serialize_attribute(&data->feed.item->attachments, attrs, "type", "description_type", 16) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else {
			if (cat_caret_to_serialization(&data->feed.item->content) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
			if (serialize_attribute(&data->feed.item->content, attrs, "type", "type", 4) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
description_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if (data->path[data->depth] == MRSS_GROUP) {
			if (cat_string_to_serialization(&data->feed.item->attachments, "description", 11, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else {
			if (cat_string_to_serialization(&data->feed.item->content, "text", 4, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_mediarss_handlers[] = {
	{"content",     MRSS_CONTENT,     &content_start,     NULL},
	{"thumbnail",   MRSS_THUMBNAIL,   &thumbnail_start,   NULL},
	{"description", MRSS_DESCRIPTION, &description_start, &description_end},
	{"group",       MRSS_GROUP,       &group_start,       NULL},
	{NULL,          XML_UNKNOWN_POS,  NULL,               NULL},
};
