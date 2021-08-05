#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

struct {
	const char *name;
	const char *column; /* name of column in database table */
	const int num;
} meta_data[] = {
	{"Feed", "feed", ITEM_COLUMN_FEED},
	{"Title", "title", ITEM_COLUMN_TITLE},
	{"Author", "author", ITEM_COLUMN_AUTHOR},
	{"Category", "category", ITEM_COLUMN_CATEGORY},
	{"Link", "url", ITEM_COLUMN_URL},
	{"Comments", "comments", ITEM_COLUMN_COMMENTS},
};

static void
cat_item_date_entry_to_buf(sqlite3_stmt *res, struct string *buf)
{
	time_t item_date = 0,
	       item_pubdate = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_PUBDATE),
	       item_upddate = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_UPDDATE);
	item_date = item_pubdate > item_upddate ? item_pubdate : item_upddate;
	if (item_date == 0) {
		return;
	}
	struct string *date_str = get_config_date_str(&item_date);
	if (date_str == NULL) {
		return;
	}
	cat_string_array(buf, "Date: ", (size_t)6);
	cat_string_string(buf, date_str);
	cat_string_char(buf, '\n');
	free_string(date_str);
}

static void
cat_item_meta_data_entry_to_buf(sqlite3_stmt *res, struct string *buf, int meta_data_index)
{
	char *text = (char *)sqlite3_column_text(res, meta_data[meta_data_index].num);
	if (text == NULL) {
		return;
	}
	size_t text_len = strlen(text);
	if (text_len == 0) {
		return;
	}
	cat_string_array(buf,
	                 (char *)meta_data[meta_data_index].name,
	                 /* strlen is optimized by compiler, right? */
	                 strlen(meta_data[meta_data_index].name));
	cat_string_array(buf, ": ", (size_t)2);
	cat_string_array(buf, text, text_len);
	cat_string_char(buf, '\n');
}

int
cat_item_meta_data_to_buf(sqlite3_stmt *res, struct string *buf)
{
	char *restrict draft_meta_data_order = malloc(sizeof(char) * (strlen(config_contents_meta_data) + 1));
	if (draft_meta_data_order == NULL) {
		debug_write(DBG_ERR, "not enough memory for tokenizing order of meta data tags\n");
		return 1; // fail
	}
	strcpy(draft_meta_data_order, config_contents_meta_data);
	char *saveptr = NULL;
	char *meta_data_entry = strtok_r(draft_meta_data_order, ",", &saveptr);
	do {
		if (strcmp(meta_data_entry, "date") == 0) {
			cat_item_date_entry_to_buf(res, buf);
			continue;
		}
		for (size_t i = 0; i < LENGTH(meta_data); ++i) {
			if (strcmp(meta_data_entry, meta_data[i].column) == 0) {
				cat_item_meta_data_entry_to_buf(res, buf, i);
				break;
			}
		}
	} while ((meta_data_entry = strtok_r(NULL, ",", &saveptr)) != NULL);
	free(draft_meta_data_order);
	return 0; // success
}
