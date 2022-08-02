#include <pthread.h>
#include "update_feed/insert_feed/insert_feed.h"

static pthread_mutex_t database_lock = PTHREAD_MUTEX_INITIALIZER;

bool
insert_feed(const struct string *url, struct getfeed_feed *feed)
{
	pthread_mutex_lock(&database_lock);

	if (db_begin_transaction() == false) {
		goto error;
	}

	if (insert_feed_data(url, feed) == false) {
		FAIL("Failed to insert feed data!");
		goto error;
	}

	struct getfeed_item *item = feed->item;
	while (item != NULL) {
		if (insert_item_data(url, item) == false) {
			FAIL("Failed to insert item data!");
			goto error;
		}
		item = item->next;
	}

	size_t items_limit = get_cfg_uint(CFG_ITEMS_COUNT_LIMIT);
	if (items_limit != 0) {
		if (delete_excess_items(url, (int64_t)items_limit) == false) {
			FAIL("Failed to delete excess items!");
		}
	}

	if (db_commit_transaction() == false) {
		goto error;
	}

	pthread_mutex_unlock(&database_lock);
	return true;
error:
	db_rollback_transaction();
	pthread_mutex_unlock(&database_lock);
	return false;
}
