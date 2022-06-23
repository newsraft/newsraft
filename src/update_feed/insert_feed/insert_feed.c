#include "update_feed/insert_feed/insert_feed.h"

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

	int64_t items_limit = (int64_t)get_cfg_uint(CFG_ITEMS_COUNT_LIMIT);
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
