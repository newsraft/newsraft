#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

struct meta_data_entry {
	const char *const prefix;   // String to write before entry data.
	const size_t prefix_len;
	const char *const suffix;   // String to write after entry data.
	const size_t suffix_len;
	const char *const column;   // Name of field to match in config_contents_meta_data.
	const enum item_column num; // Column index from which to take data.
};

static const struct meta_data_entry meta_data[] = {
	{"Feed: ",       6,  "\n", 1, "feed",       ITEM_COLUMN_FEED},
	{"Title: ",      7,  "\n", 1, "title",      ITEM_COLUMN_TITLE},
	{"Authors: ",    9,  "\n", 1, "authors",    ITEM_COLUMN_AUTHORS},
	{"Categories: ", 12, "\n", 1, "categories", ITEM_COLUMN_CATEGORIES},
	{"Link: ",       6,  "\n", 1, "url",        ITEM_COLUMN_URL},
	{"Comments: ",   10, "\n", 1, "comments",   ITEM_COLUMN_COMMENTS},
	{"Enclosures: ", 12, "\n", 1, "enclosures", ITEM_COLUMN_ENCLOSURES},
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
	char *text = (char *)sqlite3_column_text(res, meta_data[index].num);
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
	if (append_content(list, meta_data[index].prefix, meta_data[index].prefix_len, "plain", 5) != 0) {
		free_string(entry);
		return false;
	}
	if (append_content(list, entry->ptr, entry->len, "plain", 5) != 0) {
		free_string(entry);
		return false;
	}
	if (append_content(list, meta_data[index].suffix, meta_data[index].suffix_len, "plain", 5) != 0) {
		free_string(entry);
		return false;
	}
	free_string(entry);
	return true;
}

int
append_meta_data_of_item(struct content_list **list, sqlite3_stmt *res)
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
			for (size_t i = 0; i < LENGTH(meta_data); ++i) {
				if (strcmp(meta_data_entry, meta_data[i].column) == 0) {
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
