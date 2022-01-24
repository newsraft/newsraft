#include "update_feed/update_feed.h"

bool
update_feed(const struct string *url)
{
	struct getfeed_feed *feed = getfeed_url(url->ptr);
	if (feed == NULL) {
		return false;
	}

	if (db_begin_transaction() == false) {
		getfeed_free(feed);
		return false;
	}

	if (insert_feed(url, feed) == false) {
		db_rollback_transaction();
		getfeed_free(feed);
		return false;
	}

	struct getfeed_item *item = feed->item;
	while (item != NULL) {
		insert_item(url, item);
		item = item->next;
	}

	getfeed_free(feed);
	if (cfg.max_items != 0) {
		delete_excess_items(url);
	}

	if (db_commit_transaction() == false) {
		return false;
	}

	return true;
}
