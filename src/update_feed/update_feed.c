#include "update_feed/update_feed.h"

bool
update_feed(const struct string *url)
{
	bool success = false;
	struct getfeed_feed feed = {0};

	feed.etag_header = db_get_conserved_etag_header_of_the_feed(url);
	if (feed.etag_header == NULL) {
		goto undo0;
	}

	struct string *feedbuf = crtes();
	if (feedbuf == NULL) {
		goto undo1;
	}

	enum download_status status = download_feed(url->ptr, &feed, feedbuf);

	if (status == DOWNLOAD_CANCELED) {
		success = true; // Not an error.
		goto undo2;
	} else if (status == DOWNLOAD_FAILED) {
		goto undo2;
	}

	if (grow_meat_on_bones_of_the_feed(&feed) == false) {
		goto undo2;
	}

	if (parse_feed(feedbuf, &feed) == false) {
		goto undo2;
	}

	if (db_begin_transaction() == false) {
		goto undo2;
	}

	if (insert_feed(url, &feed) == false) {
		FAIL("Failed to insert \"%s\" feed!", url->ptr);
		db_rollback_transaction();
		goto undo2;
	}

	struct getfeed_item *item = feed.item;
	while (item != NULL) {
		insert_item(url, item);
		item = item->next;
	}

	if (cfg.max_items != 0) {
		delete_excess_items(url);
	}

	if (db_commit_transaction() == true) {
		success = true;
	}

undo2:
	free_string(feedbuf);
undo1:
	free_feed(&feed);
undo0:
	return success;
}
