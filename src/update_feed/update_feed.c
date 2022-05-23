#include "update_feed/update_feed.h"

bool
update_feed(const struct string *url)
{
	bool success = false;
	struct getfeed_feed feed = {0};

	feed.http_header_last_modified = db_get_date_from_feeds_table(url, "http_header_last_modified", 25);
	if (feed.http_header_last_modified == -1) {
		// Error message written by db_get_date_from_feeds_table.
		goto undo0;
	}

	feed.http_header_etag = db_get_string_from_feed_table(url, "http_header_etag", 16);
	if (feed.http_header_etag == NULL) {
		// Error message written by db_get_string_from_feed_table.
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
