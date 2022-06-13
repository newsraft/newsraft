#include <string.h>
#include "update_feed/parse_feed/xml/parse_xml_feed.h"

// Useful links:
// https://www.rssboard.org/media-rss

static inline bool
copy_type_of_mediarss_text(struct string **dest, const XML_Char **attrs)
{
	const char *type = get_value_of_attribute_key(attrs, "type");
	if (type == NULL) {
		return true; // Attribute is not set.
	}
	if (strcmp(type, "plain") == 0) {
		if (crtas_or_cpyas(dest, "text/plain", 10) == false) {
			return false;
		}
	} else if (strcmp(type, "html") == 0) {
		if (crtas_or_cpyas(dest, "text/html", 9) == false) {
			return false;
		}
	}
	return true; // Attribute doesn't meet the specification. Ignore it.
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
	size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return PARSE_OKAY; // Ignore empty content entries.
	}
	empty_link(&data->feed.temp);
	if (crtas_or_cpyas(&data->feed.temp.attachment.url, attr, attr_len) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	attr = get_value_of_attribute_key(attrs, "type");
	if (attr != NULL) {
		attr_len = strlen(attr);
		if (attr_len != 0) {
			if (crtas_or_cpyas(&data->feed.temp.attachment.type, attr, attr_len) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	attr = get_value_of_attribute_key(attrs, "fileSize");
	if (attr != NULL) {
		data->feed.temp.attachment.size = convert_string_to_size_t_or_zero(attr);
	}
	attr = get_value_of_attribute_key(attrs, "duration");
	if (attr != NULL) {
		data->feed.temp.attachment.duration = convert_string_to_size_t_or_zero(attr);
	}
	if (serialize_link(&data->feed.temp, &data->feed.item->attachments) == false) {
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
	empty_picture(&data->feed.temp);
	if (crtas_or_cpyas(&data->feed.temp.picture.url, attr, attr_len) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	attr = get_value_of_attribute_key(attrs, "width");
	if (attr != NULL) {
		data->feed.temp.picture.width = convert_string_to_size_t_or_zero(attr);
	}
	attr = get_value_of_attribute_key(attrs, "height");
	if (attr != NULL) {
		data->feed.temp.picture.height = convert_string_to_size_t_or_zero(attr);
	}
	if (serialize_picture(&data->feed.temp, &data->feed.item->pictures) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

static int8_t
description_start(struct stream_callback_data *data, const XML_Char **attrs)
{
	if (we_are_inside_item(data) == true) {
		if ((data->feed.item->content.value == NULL) || (data->feed.item->content.value->len == 0)) {
			if (copy_type_of_mediarss_text(&data->feed.item->content.type, attrs) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else if ((data->feed.item->summary.value == NULL) || (data->feed.item->summary.value->len == 0)) {
			if (copy_type_of_mediarss_text(&data->feed.item->summary.type, attrs) == false) {
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
		if ((data->feed.item->content.value == NULL) || (data->feed.item->content.value->len == 0)) {
			if (crtss_or_cpyss(&data->feed.item->content.value, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		} else if ((data->feed.item->summary.value == NULL) || (data->feed.item->summary.value->len == 0)) {
			if (crtss_or_cpyss(&data->feed.item->summary.value, data->text) == false) {
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
	{NULL,          XML_UNKNOWN_POS,  NULL,               NULL},
};
