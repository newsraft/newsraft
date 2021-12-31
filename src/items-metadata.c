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
	const enum item_column type_column; // Column index from which to take data type.
};

static const struct data_entry entries[] = {
	{"feed",       "Feed: ",       6,  "\n", 1, ITEM_COLUMN_FEED_URL,   ITEM_COLUMN_NONE},
	{"title",      "Title: ",      7,  "\n", 1, ITEM_COLUMN_TITLE,      ITEM_COLUMN_NONE},
	{"authors",    "Authors: ",    9,  "\n", 1, ITEM_COLUMN_AUTHORS,    ITEM_COLUMN_NONE},
	{"categories", "Categories: ", 12, "\n", 1, ITEM_COLUMN_CATEGORIES, ITEM_COLUMN_NONE},
	{"link",       "Link: ",       6,  "\n", 1, ITEM_COLUMN_LINK,       ITEM_COLUMN_NONE},
	{"comments",   "Comments: ",   10, "\n", 1, ITEM_COLUMN_COMMENTS,   ITEM_COLUMN_NONE},
	{"enclosures", "Enclosures: ", 12, "\n", 1, ITEM_COLUMN_ENCLOSURES, ITEM_COLUMN_NONE},
	{"summary",    "\n",           1,  "\n", 1, ITEM_COLUMN_SUMMARY,    ITEM_COLUMN_SUMMARY_TYPE},
	{"content",    "\n",           1,  "\n", 1, ITEM_COLUMN_CONTENT,    ITEM_COLUMN_CONTENT_TYPE},
};

// On success returns true.
// On failure returns false.
static inline bool
append_pubdate(struct content_list **list, sqlite3_stmt *res)
{
	time_t item_pubdate = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_PUBDATE);
	if (item_pubdate == 0) {
		return true; // It is not an error because this item simply does not have pubdate set.
	}
	struct string *pubdate_entry = create_string("Published: ", 11);
	if (pubdate_entry == NULL) {
		return false;
	}
	struct string *date_str = get_config_date_str(&item_pubdate);
	if (date_str == NULL) {
		free_string(pubdate_entry);
		return false;
	}
	if (catss(pubdate_entry, date_str) != 0) {
		free_string(date_str);
		free_string(pubdate_entry);
		return false;
	}
	free_string(date_str);
	if (catcs(pubdate_entry, '\n') != 0) {
		free_string(pubdate_entry);
		return false;
	}
	if (append_content(list, pubdate_entry->ptr, pubdate_entry->len, "plain", 5) != 0) {
		free_string(pubdate_entry);
		return false;
	}
	free_string(pubdate_entry);
	return true;
}

// On success returns true.
// On failure returns false.
static inline bool
append_upddate(struct content_list **list, sqlite3_stmt *res)
{
	time_t item_upddate = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_UPDDATE);
	if (item_upddate == 0) {
		return true; // It is not an error because this item simply does not have upddate set.
	}
	struct string *date_str = get_config_date_str(&item_upddate);
	if (date_str == NULL) {
		return false;
	}
	struct string *upddate_entry = create_string("Updated: ", 9);
	if (upddate_entry == NULL) {
		free_string(date_str);
		return false;
	}
	if (catss(upddate_entry, date_str) != 0) {
		free_string(date_str);
		free_string(upddate_entry);
		return false;
	}
	free_string(date_str);
	if (catcs(upddate_entry, '\n') != 0) {
		free_string(upddate_entry);
		return false;
	}
	if (append_content(list, upddate_entry->ptr, upddate_entry->len, "plain", 5) != 0) {
		free_string(upddate_entry);
		return false;
	}
	free_string(upddate_entry);
	return true;
}

static bool
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
	struct string *entry = create_string(text, text_len);
	if (entry == NULL) {
		return false;
	}
	if (append_content(list, entries[index].prefix, entries[index].prefix_len, "plain", 5) != 0) {
		free_string(entry);
		return false;
	}
	char *text_type;
	size_t text_type_len;
	if (entries[index].type_column != ITEM_COLUMN_NONE) {
		text_type = (char *)sqlite3_column_text(res, entries[index].type_column);
		text_type_len = strlen(text_type);
	} else {
		text_type = "";
		text_type_len = 0;
	}
	if (append_content(list, entry->ptr, entry->len, text_type, text_type_len) != 0) {
		free_string(entry);
		return false;
	}
	if (append_content(list, entries[index].suffix, entries[index].suffix_len, "plain", 5) != 0) {
		free_string(entry);
		return false;
	}
	free_string(entry);
	return true;
}

int
populate_content_list_with_data_of_item(struct content_list **list, sqlite3_stmt *res)
{
	char *restrict draft_meta_data_order = malloc(sizeof(char) * (strlen(config_contents_meta_data) + 1));
	if (draft_meta_data_order == NULL) {
		FAIL("Not enough memory for tokenizing the order of metadata entries!");
		return 1;
	}

	strcpy(draft_meta_data_order, config_contents_meta_data);

	bool error = false;

	char *saveptr = NULL;
	char *meta_data_entry = strtok_r(draft_meta_data_order, ",", &saveptr);
	while (meta_data_entry != NULL) {
		if (strcmp(meta_data_entry, "pubdate") == 0) {
			if (append_pubdate(list, res) == false) {
				error = true;
			}
		} else if (strcmp(meta_data_entry, "upddate") == 0) {
			if (append_upddate(list, res) == false) {
				error = true;
			}
		} else {
			for (size_t i = 0; i < LENGTH(entries); ++i) {
				if (strcmp(meta_data_entry, entries[i].field) == 0) {
					if (append_meta_data_entry(list, res, i) == false) {
						error = true;
					}
					break;
				}
			}
		}
		if (error == true) {
			break;
		} else {
			meta_data_entry = strtok_r(NULL, ",", &saveptr);
		}
	}

	free(draft_meta_data_order);

	if (error == true) {
		return 1;
	}

	return 0;
}
