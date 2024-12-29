#include <string.h>
#include "parse_xml/parse_xml_feed.h"

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
serialize_attribute(struct string **dest, const char *key, size_t key_len, const XML_Char **attrs, const char *attr_name)
{
	const char *val = get_value_of_attribute_key(attrs, attr_name);
	return val != NULL ? serialize_array(dest, key, key_len, val, strlen(val)) : true;
}

int8_t
generic_item_starter(struct feed_update_state *data, const XML_Char **attrs)
{
	(void)attrs;
	if (prepend_item(&data->feed.item) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	data->in_item = true;
	return PARSE_OKAY;
}

int8_t
generic_item_ender(struct feed_update_state *data)
{
	data->in_item = false;
	return PARSE_OKAY;
}

int8_t
generic_guid_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (cpyss(&data->feed.item->guid, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

int8_t
generic_title_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		if (cpyss(&data->feed.item->title, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else if (data->path[data->depth] == GENERIC_FEED) {
		if (cpyss(&data->feed.title, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

int8_t
generic_plain_content_end(struct feed_update_state *data)
{
	if (data->in_item == true) {
		if (serialize_caret(&data->feed.item->content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.item->content, "type=", 5, "text/plain", 10) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.item->content, "text=", 5, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (serialize_caret(&data->feed.content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.content, "type=", 5, "text/plain", 10) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.content, "text=", 5, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

int8_t
generic_html_content_end(struct feed_update_state *data)
{
	if (data->in_item == true) {
		if (serialize_caret(&data->feed.item->content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.item->content, "type=", 5, "text/html", 9) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.item->content, "text=", 5, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (serialize_caret(&data->feed.content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_array(&data->feed.content, "type=", 5, "text/html", 9) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.content, "text=", 5, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

int8_t
generic_category_end(struct feed_update_state *data)
{
	if (data->in_item == true) {
		if (serialize_caret(&data->feed.item->extras) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.item->extras, "category=", 9, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	} else {
		if (serialize_caret(&data->feed.extras) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (serialize_string(&data->feed.extras, "category=", 9, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

int8_t
update_date_end(struct feed_update_state *data)
{
	if (data->in_item == true) {
		data->feed.item->update_date = parse_date(data->text->ptr, true);
	}
	return PARSE_OKAY;
}

int8_t
log_xml_element_content_end(struct feed_update_state *data)
{
	INFO("Element content: %s", data->text->ptr);
	return PARSE_OKAY;
}
