#include "update_feed/update_feed.h"

static inline void
free_feed(struct getfeed_feed *feed)
{
	free_string(feed->title.value);
	free_string(feed->title.type);
	free_string(feed->url);
	free_string(feed->summary.value);
	free_string(feed->summary.type);
	free_string(feed->language);
	free_string(feed->generator.name);
	free_string(feed->generator.version);
	free_string(feed->generator.url);
	free_string(feed->rights.value);
	free_string(feed->rights.type);
	free_category(feed->category);
	free_person(feed->author);
	free_person(feed->editor);
	free_person(feed->webmaster);
	free_string(feed->http_header_etag);
	free_item(feed->item);
}

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

	if (time(&feed.download_date) == (time_t) -1) {
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
