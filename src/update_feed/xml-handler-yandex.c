#ifdef FEEDEATER_FORMAT_SUPPORT_YANDEX
#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

// https://web.archive.org/web/20211009134219/https://yandex.ru/support/news/feed.html

static inline void
fulltext_start(struct parser_data *data)
{
	data->yandex_pos |= YANDEX_FULLTEXT;
}

static inline void
fulltext_end(struct parser_data *data)
{
	if ((data->yandex_pos & YANDEX_FULLTEXT) == 0) {
		return;
	}
	data->yandex_pos &= ~YANDEX_FULLTEXT;
	if (we_are_inside_item(data) == false) {
		return;
	}
	if (data->item.content->len > data->value->len) {
		// Don't save content if it's shorter than the content we currently have.
		return;
	}
	if (cpyss(data->item.content, data->value) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
	// In most cases this is HTML...
	if (cpyas(data->item.content_type, "html", 4) == false) {
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
		// Yandex genre can only be found in item entries.
		return;
	}
	if (add_category_to_item_bucket(&data->item, data->value->ptr, data->value->len) == false) {
		data->error = PARSE_FAIL_NOT_ENOUGH_MEMORY;
		return;
	}
}

void
parse_yandex_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	(void)atts;
	     if (strcmp(name, "full-text")        == 0) { fulltext_start(data);        }
	else if (strcmp(name, "genre")            == 0) { genre_start(data);           }
	// TODO
	//else if (strcmp(name, "comment-text")     == 0) { commenttext_start(data);     }
	//else if (strcmp(name, "official-comment") == 0) { officialcomment_start(data); }
}

void
parse_yandex_element_end(struct parser_data *data, const XML_Char *name)
{
	     if (strcmp(name, "full-text")        == 0) { fulltext_end(data);        }
	else if (strcmp(name, "genre")            == 0) { genre_end(data);           }
	// TODO
	//else if (strcmp(name, "comment-text")     == 0) { commenttext_end(data);     }
	//else if (strcmp(name, "official-comment") == 0) { officialcomment_end(data); }
}
#endif // FEEDEATER_FORMAT_SUPPORT_YANDEX
