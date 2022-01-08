#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

struct data_entry {
	const char *const field;            // Name of field to match in config_contents_meta_data.
	const char *const prefix;           // String to write before entry data.
	const size_t prefix_len;
	const char *const suffix;           // String to write after entry data.
	const size_t suffix_len;
	const enum item_column data_column; // Column index from which to take data.
	const bool does_it_have_type_header_at_the_beginning;
};

static const struct data_entry entries[] = {
	{"feed",       "Feed: ",       6,  "\n", 1, ITEM_COLUMN_FEED_URL,     false},
	{"title",      "Title: ",      7,  "\n", 1, ITEM_COLUMN_TITLE,        true},
	{"authors",    "Authors: ",    9,  "\n", 1, ITEM_COLUMN_AUTHORS,      false},
	{"categories", "Categories: ", 12, "\n", 1, ITEM_COLUMN_CATEGORIES,   false},
	{"link",       "Link: ",       6,  "\n", 1, ITEM_COLUMN_LINK,         false},
	{"comments",   "Comments: ",   10, "\n", 1, ITEM_COLUMN_COMMENTS_URL, false},
	{"summary",    "\n\n",         2,  "\n", 1, ITEM_COLUMN_SUMMARY,      true},
	{"content",    "\n\n",         2,  "\n", 1, ITEM_COLUMN_CONTENT,      true},
};

// On success returns true.
// On failure returns false.
static inline bool
append_date(struct content_list **list, sqlite3_stmt *res, intmax_t column, const char *prefix, size_t prefix_len)
{
	time_t date = (time_t)sqlite3_column_int64(res, column);
	if (date == 0) {
		return true; // It is not an error because this item simply does not have pubdate set.
	}
	struct string *date_entry = crtas(prefix, prefix_len);
	if (date_entry == NULL) {
		return false;
	}
	struct string *date_str = get_config_date_str(date);
	if (date_str == NULL) {
		free_string(date_entry);
		return false;
	}
	if (catss(date_entry, date_str) == false) {
		free_string(date_str);
		free_string(date_entry);
		return false;
	}
	free_string(date_str);
	if (catcs(date_entry, '\n') == false) {
		free_string(date_entry);
		return false;
	}
	if (append_content(list, date_entry->ptr, date_entry->len, "text/plain", 10, false) == false) {
		free_string(date_entry);
		return false;
	}
	free_string(date_entry);
	return true;
}

static inline bool
append_meta_data_entry(struct content_list **list, sqlite3_stmt *res, int index)
{
	char *text = (char *)sqlite3_column_text(res, entries[index].data_column);
	if (text == NULL) {
		return true; // It is not an error because this item simply does not have value set.
	}
	size_t text_len = strlen(text);
	if (text_len == 0) {
		return true; // It is not an error because this item simply does not have value set.
	}

	if (append_content(list, entries[index].prefix, entries[index].prefix_len, "text/plain", 10, false) == false) {
		return false;
	}

	if (entries[index].does_it_have_type_header_at_the_beginning == true) {
		char *type_separator = strchr(text, ';');
		if (type_separator == NULL) {
			return false;
		}
		size_t type_len = type_separator - text;
		if (type_len > MAX_MIME_TYPE_LEN) {
			return false;
		}
		const char *real_text = text + (type_len + 1);
		size_t real_text_len = text_len - (type_len + 1);
		char type[MAX_MIME_TYPE_LEN + 1];
		memcpy(type, text, type_len);
		type[type_len] = '\0';
		if (append_content(list, real_text, real_text_len, type, type_len, true) == false) {
			return false;
		}
	} else {
		if (append_content(list, text, text_len, "text/plain", 10, true) == false) {
			return false;
		}
	}

	if (append_content(list, entries[index].suffix, entries[index].suffix_len, "text/plain", 10, false) == false) {
		return false;
	}

	return true;
}

static inline bool
append_max_summary_content(struct content_list **list, sqlite3_stmt *res)
{
	const char *summary = (char *)sqlite3_column_text(res, ITEM_COLUMN_SUMMARY);
	const char *content = (char *)sqlite3_column_text(res, ITEM_COLUMN_CONTENT);
	const size_t summary_len = strlen(summary);
	size_t content_len = strlen(content);
	if (summary_len > content_len) {
		content = summary;
		content_len = summary_len;
	}
	if (content == NULL) {
		return true;
	}
	if (content_len == 0) {
		return true;
	}
	const char *type_separator = strchr(content, ';');
	if (type_separator == NULL) {
		return false;
	}
	const size_t type_len = type_separator - content;
	if (type_len > MAX_MIME_TYPE_LEN) {
		return false;
	}
	const char *real_content = content + (type_len + 1);
	const size_t real_content_len = content_len - (type_len + 1);
	char type[MAX_MIME_TYPE_LEN + 1];
	memcpy(type, content, type_len);
	type[type_len] = '\0';
	if (append_content(list, "\n", 1, "text/plain", 10, false) == false) {
		return false;
	}
	if (append_content(list, real_content, real_content_len, type, type_len, true) == false) {
		return false;
	}
	return true;
}

static inline bool
process_specifier(const struct string *value, struct content_list **list, sqlite3_stmt *res)
{
	if (strcmp(value->ptr, "published") == 0) {
		if (append_date(list, res, ITEM_COLUMN_PUBDATE, "Published: ", 11) == false) {
			return false;
		}
	} else if (strcmp(value->ptr, "updated") == 0) {
		if (append_date(list, res, ITEM_COLUMN_UPDDATE, "Updated: ", 9) == false) {
			return false;
		}
	} else if (strcmp(value->ptr, "max-summary-content") == 0) {
		if (append_max_summary_content(list, res) == false) {
			return false;
		}
	} else {
		for (size_t i = 0; i < COUNTOF(entries); ++i) {
			if (strcmp(value->ptr, entries[i].field) == 0) {
				if (append_meta_data_entry(list, res, i) == false) {
					return false;
				}
				break;
			}
		}
	}
	return true;
}

bool
populate_content_list_with_data_of_item(struct content_list **list, sqlite3_stmt *res)
{
	struct string *value = crtes();
	if (value == NULL) {
		return false;
	}
	const char *i = cfg.contents_meta_data;
	while (true) {
		if ((*i == ',') || (*i == '\0')) {
			if (process_specifier(value, list, res) == false) {
				goto error;
			}
			empty_string(value);
		} else {
			if (catcs(value, *i) == false) {
				goto error;
			}
		}
		if (*i == '\0') {
			break;
		}
		++i;
	}
	free_string(value);
	return true;
error:
	free_string(value);
	return false;
}
