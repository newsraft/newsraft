#include "update_feed/parse_feed/xml/parse_xml_feed.h"

// https://yandex.ru/support/news/feed.html

static int8_t
full_text_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		// Save this content only if it's longer than the content we currently have.
		if ((data->feed.item->content.value == NULL) || (data->text->len > data->feed.item->content.value->len)) {
			if (crtss_or_cpyss(&data->feed.item->content.value, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
			// In most cases it is HTML.
			if (crtas_or_cpyas(&data->feed.item->content.type, "text/html", 9) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
comment_text_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		// Save this summary only if it's longer than the summary we currently have.
		if ((data->feed.item->summary.value == NULL) || (data->text->len > data->feed.item->summary.value->len)) {
			if (crtss_or_cpyss(&data->feed.item->summary.value, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
			// Specification says that all comments are plain text.
			if (crtas_or_cpyas(&data->feed.item->summary.type, "text/plain", 10) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
		}
	}
	return PARSE_OKAY;
}

static int8_t
genre_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
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
