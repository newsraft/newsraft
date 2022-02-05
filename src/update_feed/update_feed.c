#include "update_feed/update_feed.h"

bool
update_feed(const struct string *url)
{
	struct string *feedbuf = download_feed(url->ptr);
	if (feedbuf == NULL) {
		// Just a warning because download can fail when there's no internet.
		WARN("Failed to download a feed!");
		return false;
	}

	struct getfeed_feed *feed = parse_feed(feedbuf);
	if (feed == NULL) {
		free_string(feedbuf);
		return false;
	}

	free_string(feedbuf);

	if (db_begin_transaction() == false) {
		free_feed(feed);
		return false;
	}

	if (insert_feed(url, feed) == false) {
		db_rollback_transaction();
		free_feed(feed);
		return false;
	}

	struct getfeed_item *item = feed->item;
	while (item != NULL) {
		insert_item(url, item);
		item = item->next;
	}

	free_feed(feed);
	if (cfg.max_items != 0) {
		delete_excess_items(url);
	}

	if (db_commit_transaction() == false) {
		return false;
	}

	return true;
}
