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

static int
cat_item_date_entry_to_buf(struct string *buf, sqlite3_stmt *res)
{
	time_t item_pubdate = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_PUBDATE);
	time_t item_upddate = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_UPDDATE);
	time_t item_date = item_pubdate > item_upddate ? item_pubdate : item_upddate;
	if (item_date == 0) {
		return 0; // success. it is not an error because this item simply does not have date set
	}
	struct string *date_str = get_config_date_str(&item_date);
	if (date_str == NULL) {
		return 1; // failure
	}
	catas(buf, "Date: ", (size_t)6);
	catss(buf, date_str);
	catcs(buf, '\n');
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
