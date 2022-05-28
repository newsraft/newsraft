#ifdef NEWSRAFT_FORMAT_SUPPORT_YANDEX
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// https://web.archive.org/web/20211009134219/https://yandex.ru/support/news/feed.html

static void
full_text_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == false) {
		return;
	}
	if ((data->feed.item->content.value != NULL) && (data->value->len < data->feed.item->content.value->len)) {
		// Don't save content if it's shorter than the content we currently have.
		return;
	}
	if (crtss_or_cpyss(&data->feed.item->content.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	// In most cases this is HTML.
	if (crtas_or_cpyas(&data->feed.item->content.type, "text/html", 9) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
genre_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == false) {
		return;
	}
	if (prepend_category(&data->feed.item->category) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (crtss_or_cpyss(&data->feed.item->category->term, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
comment_text_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == false) {
		return;
	}
	if ((data->feed.item->summary.value != NULL) && (data->value->len < data->feed.item->summary.value->len)) {
		// Don't save summary if it's shorter than the summary we currently have.
		return;
	}
	if (crtss_or_cpyss(&data->feed.item->summary.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	// Spec says that all comments are plain text.
	if (crtas_or_cpyas(&data->feed.item->summary.type, "text/plain", 10) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static void
bind_to_end(struct stream_callback_data *data)
{
	if (we_are_inside_item(data) == false) {
		return;
	}
	if (prepend_link(&data->feed.item->attachment) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (crtss_or_cpyss(&data->feed.item->attachment->url, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

const struct xml_element_handler xml_yandex_handlers[] = {
	// <official-comment> is a container for most of this stuff but it is quite redundant.
	{"full-text",    YANDEX_FULL_TEXT,    NULL, &full_text_end},
	{"genre",        YANDEX_GENRE,        NULL, &genre_end},
	{"comment-text", YANDEX_COMMENT_TEXT, NULL, &comment_text_end},
	{"bind-to",      YANDEX_BIND_TO,      NULL, &bind_to_end},
	{NULL,           YANDEX_NONE,         NULL, NULL},
};
#endif // NEWSRAFT_FORMAT_SUPPORT_YANDEX
