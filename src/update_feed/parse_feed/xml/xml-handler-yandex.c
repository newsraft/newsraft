#include "update_feed/parse_feed/xml/parse_xml_feed.h"

// https://yandex.ru/support/news/feed.html

static int8_t
full_text_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if (cat_caret_to_serialization(&data->feed.item->content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_array_to_serialization(&data->feed.item->content, "type", 4, "text/html", 9) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.item->content, "text", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
comment_text_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if (cat_caret_to_serialization(&data->feed.item->content) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_array_to_serialization(&data->feed.item->content, "type", 4, "text/plain", 10) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.item->content, "text", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
genre_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if (cat_caret_to_serialization(&data->feed.item->categories) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.item->categories, "term", 4, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
bind_to_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if (cat_caret_to_serialization(&data->feed.item->attachments) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		if (cat_string_to_serialization(&data->feed.item->attachments, "url", 3, data->text) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

const struct xml_element_handler xml_yandex_handlers[] = {
	// <official-comment> is a container for most of this stuff but it is quite redundant.
	{"full-text",    YANDEX_FULL_TEXT,    NULL, &full_text_end},
	{"comment-text", YANDEX_COMMENT_TEXT, NULL, &comment_text_end},
	{"genre",        YANDEX_GENRE,        NULL, &genre_end},
	{"bind-to",      YANDEX_BIND_TO,      NULL, &bind_to_end},
	{NULL,           XML_UNKNOWN_POS,     NULL, NULL},
};
