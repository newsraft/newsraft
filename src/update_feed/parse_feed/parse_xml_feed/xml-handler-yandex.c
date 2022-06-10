#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// https://web.archive.org/web/20211009134219/https://yandex.ru/support/news/feed.html

static int8_t
full_text_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		// Save content only if it's longer than the content we currently have.
		if ((data->feed.item->content.value == NULL) || (data->text->len > data->feed.item->content.value->len)) {
			if (crtss_or_cpyss(&data->feed.item->content.value, data->text) == false) {
				return PARSE_FAIL_NOT_ENOUGH_MEMORY;
			}
			// In most cases this is HTML.
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
	if (we_are_inside_item(data) == false) {
		return PARSE_OKAY;
	}
	if ((data->feed.item->summary.value != NULL) && (data->text->len < data->feed.item->summary.value->len)) {
		// Don't save summary if it's shorter than the summary we currently have.
		return PARSE_OKAY;
	}
	if (crtss_or_cpyss(&data->feed.item->summary.value, data->text) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	// Specification says that all comments are plain text.
	if (crtas_or_cpyas(&data->feed.item->summary.type, "text/plain", 10) == false) {
		return PARSE_FAIL_NOT_ENOUGH_MEMORY;
	}
	return PARSE_OKAY;
}

static int8_t
genre_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if (prepend_category(&data->feed.item->category) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		data->feed.item->category->term = crtss(data->text);
		if (data->feed.item->category->term == NULL) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
	}
	return PARSE_OKAY;
}

static int8_t
bind_to_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == true) {
		if (prepend_link(&data->feed.item->attachment) == false) {
			return PARSE_FAIL_NOT_ENOUGH_MEMORY;
		}
		data->feed.item->attachment->url = crtss(data->text);
		if (data->feed.item->attachment->url == NULL) {
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
