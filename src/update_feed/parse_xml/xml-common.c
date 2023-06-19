#include <string.h>
#include "update_feed/parse_xml/parse_xml_feed.h"

const char *
get_value_of_attribute_key(const XML_Char **attrs, const char *key)
{
	for (size_t i = 0; attrs[i] != NULL; i += 2) {
		if (strcmp(key, attrs[i]) == 0) {
			return attrs[i + 1];
		}
	}
	return NULL;
}

bool
serialize_attribute(struct string **dest, const XML_Char **attrs, const char *attr_key, const char *prefix, size_t prefix_len)
{
	const char *attr_value = get_value_of_attribute_key(attrs, attr_key);
	if (attr_value == NULL) {
		return true; // Ignore absence of an attribute.
	}
	const size_t attr_value_len = strlen(attr_value);
	if (attr_value_len == 0) {
		return true; // Ignore empty attribute values.
	}
	return serialize_array(dest, prefix, prefix_len, attr_value, attr_value_len);
}

int8_t
generic_item_starter(struct stream_callback_data *data, const XML_Char **attrs)
{
	(void)attrs;
	if (prepend_item(&data->feed.item) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	data->in_item = true;
	return PARSE_OKAY;
}

int8_t
generic_item_ender(struct stream_callback_data *data)
{
	data->in_item = false;
	return PARSE_OKAY;
}

int8_t
generic_guid_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (crtss_or_cpyss(&data->feed.item->guid, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (data->feed.item->guid_is_url == true && data->feed.item->url == NULL) {
			if (crtss_or_cpyss(&data->feed.item->url, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

int8_t
generic_title_end(struct stream_callback_data *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (crtss_or_cpyss(&data->feed.item->title, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == GENERIC_FEED) {
		if (crtss_or_cpyss(&data->feed.title, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

int8_t
generic_plain_content_end(struct stream_callback_data *data)
{
	if (data->in_item == true) {
		if (serialize_caret(&data->feed.item->content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.item->content, "type", 4, "text/plain", 10) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.item->content, "text", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (serialize_caret(&data->feed.content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.content, "type", 4, "text/plain", 10) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.content, "text", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

int8_t
generic_html_content_end(struct stream_callback_data *data)
{
	if (data->in_item == true) {
		if (serialize_caret(&data->feed.item->content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.item->content, "type", 4, "text/html", 9) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.item->content, "text", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (serialize_caret(&data->feed.content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.content, "type", 4, "text/html", 9) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.content, "text", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

int8_t
generic_category_end(struct stream_callback_data *data)
{
	if (data->in_item == true) {
		if (serialize_caret(&data->feed.item->extras) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.item->extras, "category", 8, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (serialize_caret(&data->feed.extras) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.extras, "category", 8, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

int8_t
generator_end(struct stream_callback_data *data)
{
	INFO("Feed generator name: %s", data->text->ptr);
	return PARSE_OKAY;
}
