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

void
serialize_attribute(struct string **dest, const char *key, size_t key_len, const XML_Char **attrs, const char *attr_name)
{
	const char *val = get_value_of_attribute_key(attrs, attr_name);
	if (val) {
		serialize_array(dest, key, key_len, val, strlen(val));
	}
}

void
generic_item_starter(struct feed_update_state *data, const XML_Char **attrs)
{
	(void)attrs;
	prepend_item(&data->feed.item);
	data->in_item = true;
}

void
generic_item_ender(struct feed_update_state *data)
{
	data->in_item = false;
}

void
generic_guid_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		cpyss(&data->feed.item->guid, data->text);
	}
}

void
generic_title_end(struct feed_update_state *data)
{
	if (data->path[data->depth] == GENERIC_ITEM) {
		cpyss(&data->feed.item->title, data->text);
	} else if (data->path[data->depth] == GENERIC_FEED) {
		cpyss(&data->feed.title, data->text);
	}
}

void
generic_plain_content_end(struct feed_update_state *data)
{
	struct string **dest = data->in_item ? &data->feed.item->content : &data->feed.content;
	serialize_caret(dest);
	serialize_array(dest, "type=", 5, "text/plain", 10);
	serialize_string(dest, "text=", 5, data->text);
}

void
generic_html_content_end(struct feed_update_state *data)
{
	struct string **dest = data->in_item ? &data->feed.item->content : &data->feed.content;
	serialize_caret(dest);
	serialize_array(dest, "type=", 5, "text/html", 9);
	serialize_string(dest, "text=", 5, data->text);
}

void
generic_category_end(struct feed_update_state *data)
{
	struct string **dest = data->in_item ? &data->feed.item->extras : &data->feed.extras;
	serialize_caret(dest);
	serialize_string(dest, "category=", 9, data->text);
}

void
update_date_end(struct feed_update_state *data)
{
	if (data->in_item) {
		data->feed.item->update_date = parse_date(data->text->ptr, true);
	}
}

void
log_xml_element_content_end(struct feed_update_state *data)
{
	INFO("Element content: %s", data->text->ptr);
}
