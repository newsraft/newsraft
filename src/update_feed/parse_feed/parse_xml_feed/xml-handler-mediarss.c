#ifdef FEEDEATER_FORMAT_SUPPORT_MEDIARSS
#include <string.h>
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// Useful links:
// https://www.rssboard.org/media-rss

static void
content_start(struct xml_data *data, const TidyAttr attrs)
{
	if (we_are_inside_item(data) == false) {
		return; // <media:content> is a sub-element of either <item> or <media:group>.
	}
	const char *url = get_value_of_attribute_key(attrs, "url");
	if (url == NULL) {
		return; // Ignore empty content entries.
	}
	const size_t url_len = strlen(url);
	if (url_len == 0) {
		return; // Ignore empty content entries.
	}
	if (prepend_link(&data->feed->item->attachment) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (crtas_or_cpyas(&data->feed->item->attachment->url, url, url_len) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	const char *type = get_value_of_attribute_key(attrs, "type");
	if (type != NULL) {
		if (crtas_or_cpyas(&data->feed->item->attachment->type, type, strlen(type)) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
	const char *length = get_value_of_attribute_key(attrs, "fileSize");
	if (length != NULL) {
		data->feed->item->attachment->size = convert_string_to_size_t_or_zero(length);
	}
	const char *duration = get_value_of_attribute_key(attrs, "duration");
	if (duration != NULL) {
		data->feed->item->attachment->duration = convert_string_to_size_t_or_zero(duration);
	}
}

static void
thumbnail_start(struct xml_data *data, const TidyAttr attrs)
{
	if (we_are_inside_item(data) == false) {
		return;
	}
	const char *attr = get_value_of_attribute_key(attrs, "url");
	if (attr == NULL) {
		return; // Ignore empty content entries.
	}
	const size_t attr_len = strlen(attr);
	if (attr_len == 0) {
		return; // Ignore empty content entries.
	}
	if (prepend_empty_picture(&data->feed->item->thumbnail) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (crtas_or_cpyas(&data->feed->item->thumbnail->url, attr, attr_len) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	attr = get_value_of_attribute_key(attrs, "width");
	if (attr != NULL) {
		data->feed->item->thumbnail->width = convert_string_to_size_t_or_zero(attr);
	}
	attr = get_value_of_attribute_key(attrs, "height");
	if (attr != NULL) {
		data->feed->item->thumbnail->height = convert_string_to_size_t_or_zero(attr);
	}
}

static void
description_end(struct xml_data *data, const TidyAttr attrs)
{
	if (we_are_inside_item(data) == false) {
		return;
	}
	if ((data->feed->item->content.value == NULL) || (data->feed->item->content.value->len == 0)) {
		if (crtss_or_cpyss(&data->feed->item->content.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (copy_type_of_text_construct(&data->feed->item->content.type, attrs) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	} else if ((data->feed->item->summary.value == NULL) || (data->feed->item->summary.value->len == 0)) {
		if (crtss_or_cpyss(&data->feed->item->summary.value, data->value) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
		if (copy_type_of_text_construct(&data->feed->item->summary.type, attrs) == false) {
			data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
			return;
		}
	}
}

const struct xml_element_handler xml_mediarss_handlers[] = {
	{"content",     MRSS_CONTENT,     &content_start,     NULL},
	{"thumbnail",   MRSS_THUMBNAIL,   &thumbnail_start,   NULL},
	{"description", MRSS_DESCRIPTION, NULL,               &description_end},
	{NULL,          MRSS_NONE,        NULL,               NULL},
};
#endif // FEEDEATER_FORMAT_SUPPORT_MEDIARSS
