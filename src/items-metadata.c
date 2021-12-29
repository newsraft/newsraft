#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

struct meta_data_entry {
	const char *const prefix;
	const size_t prefix_len;
	const char *const column; /* name of corresponding column in database's items table */
	const enum item_column num;
};

static const struct meta_data_entry meta_data[] = {
	{"Feed: ",       6,  "feed",       ITEM_COLUMN_FEED},
	{"Title: ",      7,  "title",      ITEM_COLUMN_TITLE},
	{"Authors: ",    9,  "authors",    ITEM_COLUMN_AUTHORS},
	{"Categories: ", 12, "categories", ITEM_COLUMN_CATEGORIES},
	{"Link: ",       6,  "url",        ITEM_COLUMN_URL},
	{"Comments: ",   10, "comments",   ITEM_COLUMN_COMMENTS},
	{"Enclosures: ", 12, "enclosures", ITEM_COLUMN_ENCLOSURES},
};

// On success returns true.
// On failure returns false.
static inline bool
cat_item_pubdate_to_buf(struct string *buf, sqlite3_stmt *res)
{
	time_t item_pubdate = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_PUBDATE);
	if (item_pubdate == 0) {
		return true; // It is not an error because this item simply does not have pubdate set.
	}
	struct string *date_str = get_config_date_str(&item_pubdate);
	if (date_str == NULL) {
		return false;
	}
	if (catas(buf, "Published: ", (size_t)11) != 0) {
		free_string(date_str);
		return false;
	}
	if (catss(buf, date_str) != 0) {
		free_string(date_str);
		return false;
	}
	free_string(date_str);
	if (catcs(buf, '\n') != 0) {
		return false;
	}
	return true;
}

// On success returns true.
// On failure returns false.
static inline bool
cat_item_upddate_to_buf(struct string *buf, sqlite3_stmt *res)
{
	time_t item_upddate = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_UPDDATE);
	if (item_upddate == 0) {
		return true; // It is not an error because this item simply does not have upddate set.
	}
	struct string *date_str = get_config_date_str(&item_upddate);
	if (date_str == NULL) {
		return false;
	}
	if (catas(buf, "Updated: ", (size_t)9) != 0) {
		free_string(date_str);
		return false;
	}
	if (catss(buf, date_str) != 0) {
		free_string(date_str);
		return false;
	}
	free_string(date_str);
	if (catcs(buf, '\n') != 0) {
		return false;
	}
	return true;
}

static int
cat_item_meta_data_entry_to_buf(struct string *buf, sqlite3_stmt *res, int meta_data_index)
{
	char *text = (char *)sqlite3_column_text(res, meta_data[meta_data_index].num);
	if (text == NULL) {
		return 0; // success. it is not an error because this item simply does not have that meta data entry
	}
	size_t text_len = strlen(text);
	if (text_len == 0) {
		return 0; // success. it is not an error because this item simply does not have that meta data entry
	}
	// TODO: add error checks for strings concatenation
	catas(buf, meta_data[meta_data_index].prefix, meta_data[meta_data_index].prefix_len);
	catas(buf, text, text_len);
	catcs(buf, '\n');
	return 0; // success
}

struct string *
get_meta_data_of_item(sqlite3_stmt *res)
{
	struct string *buf = create_string(NULL, 100);
	if (buf == NULL) {
		return NULL;
	}

	char *restrict draft_meta_data_order = malloc(sizeof(char) * (strlen(config_contents_meta_data) + 1));
	if (draft_meta_data_order == NULL) {
		FAIL("Not enough memory for tokenizing the order of metadata entries!");
		free_string(buf);
		return NULL;
	}

	strcpy(draft_meta_data_order, config_contents_meta_data);

	bool error = false;

	char *saveptr = NULL;
	char *meta_data_entry = strtok_r(draft_meta_data_order, ",", &saveptr);
	while (meta_data_entry != NULL) {
		if (strcmp(meta_data_entry, "pubdate") == 0) {
			if (cat_item_pubdate_to_buf(buf, res) == false) {
				error = true;
			}
		} else if (strcmp(meta_data_entry, "upddate") == 0) {
			if (cat_item_upddate_to_buf(buf, res) == false) {
				error = true;
			}
		} else {
			for (size_t i = 0; i < LENGTH(meta_data); ++i) {
				if (strcmp(meta_data_entry, meta_data[i].column) == 0) {
					if (cat_item_meta_data_entry_to_buf(buf, res, i) != 0) {
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
		free_string(buf);
		return NULL;
	}

	return buf;
}
