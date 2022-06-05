#include "update_feed/insert_feed/insert_feed.h"

bool
db_bind_text_struct(sqlite3_stmt *s, int8_t placeholder, struct getfeed_text *text_struct)
{
	if ((text_struct->value == NULL) || (text_struct->value->len == 0)) {
		sqlite3_bind_null(s, placeholder);
		return true;
	}
	if (text_struct->type == NULL) {
		text_struct->type = crtas("text/plain", 10);
		if (text_struct->type == NULL) {
			return false;
		}
	} else if (text_struct->type->len == 0) {
		if (cpyas(text_struct->type, "text/plain", 10) == false) {
			return false;
		}
	}
	if (catcs(text_struct->type, ';') == false) {
		return false;
	}
	if (catss(text_struct->type, text_struct->value) == false) {
		return false;
	}
	if (db_bind_string(s, placeholder, text_struct->type) != SQLITE_OK) {
		return false;
	}
	return true;
}

bool
insert_feed(const struct string *url, struct getfeed_feed *feed)
{
	if (db_begin_transaction() == false) {
		return false;
	}

	if (insert_feed_data(url, feed) == false) {
		FAIL("Failed to insert feed data!");
		db_rollback_transaction();
		return false;
	}

	struct getfeed_item *item = feed->item;
	while (item != NULL) {
		if (insert_item_data(url, item) == false) {
			FAIL("Failed to insert item data!");
			db_rollback_transaction();
			return false;
		}
		item = item->next;
	}

	int64_t items_limit = (int64_t)get_cfg_uint(CFG_MAX_ITEMS);
	if (items_limit > 0) {
		if (delete_excess_items(url, items_limit) == false) {
			FAIL("Failed to delete excess items!");
		}
	}

	if (db_commit_transaction() == false) {
		db_rollback_transaction();
		return false;
	}

	return true;
}
