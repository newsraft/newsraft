#include "update_feed/insert_feed/insert_feed.h"

bool
db_bind_text_struct(sqlite3_stmt *s, intmax_t placeholder, const struct getfeed_text *text_struct)
{
	if (text_struct->value->len == 0) {
		sqlite3_bind_text(s, placeholder, "", 0, NULL);
		return true;
	}
	if (catcs((struct string *)text_struct->type, ';') == false) {
		return false;
	}
	if (catss((struct string *)text_struct->type, (struct string *)text_struct->value) == false) {
		return false;
	}
	if (sqlite3_bind_text(s, placeholder, text_struct->type->ptr, text_struct->type->len, NULL) != SQLITE_OK) {
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

	if (get_cfg_uint(CFG_MAX_ITEMS) != 0) {
		delete_excess_items(url);
	}

	if (db_commit_transaction() == false) {
		db_rollback_transaction();
		return false;
	}

	return true;
}
