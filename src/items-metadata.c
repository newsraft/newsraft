#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

struct meta_data_entry {
	const char *const name;
	const size_t name_len;
	const char *const column; /* name of corresponding column in database's items table */
	const enum item_column num;
};

const struct meta_data_entry meta_data[] = {
	{"Feed",       4,  "feed",       ITEM_COLUMN_FEED},
	{"Title",      5,  "title",      ITEM_COLUMN_TITLE},
	{"Authors",    7,  "authors",    ITEM_COLUMN_AUTHORS},
	{"Categories", 10, "categories", ITEM_COLUMN_CATEGORIES},
	{"Link",       4,  "url",        ITEM_COLUMN_URL},
	{"Comments",   8,  "comments",   ITEM_COLUMN_COMMENTS},
};

static int
cat_item_date_entry_to_buf(struct string *buf, sqlite3_stmt *res)
{
	time_t item_date = 0,
	       item_pubdate = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_PUBDATE),
	       item_upddate = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_UPDDATE);
	item_date = item_pubdate > item_upddate ? item_pubdate : item_upddate;
	if (item_date == 0) {
		return 0; // success. it is not an error because this item simply does not have date set
	}
	struct string *date_str = get_config_date_str(&item_date);
	if (date_str == NULL) {
		return 1; // failure
	}
	cat_string_array(buf, "Date: ", (size_t)6);
	cat_string_string(buf, date_str);
	cat_string_char(buf, '\n');
	free_string(date_str);
	return 0; // success
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
	cat_string_array(buf, (char *)meta_data[meta_data_index].name, meta_data[meta_data_index].name_len);
	cat_string_array(buf, ": ", (size_t)2);
	cat_string_array(buf, text, text_len);
	cat_string_char(buf, '\n');
	return 0; // success
}

int
cat_item_meta_data_to_buf(struct string *buf, sqlite3_stmt *res)
{
	char *restrict draft_meta_data_order = malloc(sizeof(char) * (strlen(config_contents_meta_data) + 1));
	if (draft_meta_data_order == NULL) {
		FAIL("Not enough memory for tokenizing the order of metadata entries!");
		return 1; // failure
	}

	strcpy(draft_meta_data_order, config_contents_meta_data);

	bool error = false;
	char *saveptr = NULL;
	char *meta_data_entry = strtok_r(draft_meta_data_order, ",", &saveptr);
	do {
		if (strcmp(meta_data_entry, "date") == 0) {
			if (cat_item_date_entry_to_buf(buf, res) != 0) {
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
		}
	} while ((meta_data_entry = strtok_r(NULL, ",", &saveptr)) != NULL);

	free(draft_meta_data_order);

	if (error == true) {
		return 1; // failure
	}

	return 0; // success
}
