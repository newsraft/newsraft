#include "update_feed/update_feed.h"

bool
update_feed(const struct string *url)
{
	bool success = false;
	struct getfeed_feed feed = {0};

	feed.last_modified_header = db_get_string_from_feed_table(url, "last_modified_header", 20);
	if (feed.last_modified_header == NULL) {
		goto undo0;
	}

	feed.etag_header = db_get_string_from_feed_table(url, "etag_header", 11);
	if (feed.etag_header == NULL) {
		goto undo1;
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

	if (initialize_feed(&feed) == false) {
		goto undo2;
	}

	if (parse_feed(feedbuf, &feed) == false) {
		goto undo2;
	}

	success = insert_feed(url, &feed);

undo2:
	free_string(feedbuf);
undo1:
	free_feed(&feed);
undo0:
	return success;
}
