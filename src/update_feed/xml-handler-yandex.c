#ifdef FEEDEATER_FORMAT_SUPPORT_YANDEX
#include <string.h>
#include "update_feed/update_feed.h"

// https://web.archive.org/web/20211009134219/https://yandex.ru/support/news/feed.html

static inline void
full_text_start(struct parser_data *data)
{
	data->yandex_pos |= YANDEX_FULL_TEXT;
}

static inline void
full_text_end(struct parser_data *data)
{
	if ((data->yandex_pos & YANDEX_FULL_TEXT) == 0) {
		return;
	}
	data->yandex_pos &= ~YANDEX_FULL_TEXT;
	if (we_are_inside_item(data) == false) {
		return;
	}
	if (data->value->len < data->item.content.value->len) {
		// Don't save content if it's shorter than the content we currently have.
		return;
	}
	if (cpyss(data->item.content.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	// In most cases this is HTML.
	if (cpyas(data->item.content.type, "text/html", 9) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
genre_start(struct parser_data *data)
{
	data->yandex_pos |= YANDEX_GENRE;
}

static inline void
genre_end(struct parser_data *data)
{
	if ((data->yandex_pos & YANDEX_GENRE) == 0) {
		return;
	}
	data->yandex_pos &= ~YANDEX_GENRE;
	if (we_are_inside_item(data) == false) {
		return;
	}
	if (add_category_to_item_bucket(&data->item, data->value->ptr, data->value->len) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
comment_text_start(struct parser_data *data, const XML_Char **atts)
{
	(void)atts; // TODO read origin attribute
	data->yandex_pos |= YANDEX_COMMENT_TEXT;
}

static inline void
comment_text_end(struct parser_data *data)
{
	if ((data->yandex_pos & YANDEX_COMMENT_TEXT) == 0) {
		return;
	}
	data->yandex_pos &= ~YANDEX_COMMENT_TEXT;
	if (we_are_inside_item(data) == false) {
		return;
	}
	if (data->value->len < data->item.summary.value->len) {
		// Don't save summary if it's shorter than the summary we currently have.
		return;
	}
	if (cpyss(data->item.summary.value, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	// Spec says that all comments are plain text.
	if (cpyas(data->item.summary.type, "text/plain", 10) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

static inline void
bind_to_start(struct parser_data *data)
{
	data->yandex_pos |= YANDEX_BIND_TO;
}

static inline void
bind_to_end(struct parser_data *data)
{
	if ((data->yandex_pos & YANDEX_BIND_TO) == 0) {
		return;
	}
	data->yandex_pos &= ~YANDEX_BIND_TO;
	if (we_are_inside_item(data) == false) {
		return;
	}
	if (expand_link_list_by_one_element(&data->item.attachments) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	if (add_url_to_last_link(&data->item.attachments, data->value->ptr, data->value->len) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

void
parse_yandex_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	(void)atts;
	     if (strcmp(name, "full-text")    == 0) { full_text_start(data);          }
	else if (strcmp(name, "genre")        == 0) { genre_start(data);              }
	else if (strcmp(name, "comment-text") == 0) { comment_text_start(data, atts); }
	else if (strcmp(name, "bind-to")      == 0) { bind_to_start(data);            }
	// official-comment is a container for most of this stuff but it is quite redundant.
	//else if (strcmp(name, "official-comment") == 0) { /*      ignore it      */ }
}

void
parse_yandex_element_end(struct parser_data *data, const XML_Char *name)
{
	     if (strcmp(name, "full-text")    == 0) { full_text_end(data);    }
	else if (strcmp(name, "genre")        == 0) { genre_end(data);        }
	else if (strcmp(name, "comment-text") == 0) { comment_text_end(data); }
	else if (strcmp(name, "bind-to")      == 0) { bind_to_end(data);      }
	// official-comment is a container for most of this stuff but it is quite redundant.
	//else if (strcmp(name, "official-comment") == 0) { /*  ignore it  */ }
}
#endif // FEEDEATER_FORMAT_SUPPORT_YANDEX
