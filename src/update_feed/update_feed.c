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
	struct stream_callback_data data = {0};

	if (time(&data.feed.download_date) == (time_t) -1) {
		goto undo0;
	}

	if (get_cfg_bool(CFG_RESPECT_EXPIRES_HEADER) == true) {
		int64_t expires_date = db_get_date_from_feeds_table(url, "http_header_expires", 19);
		if (expires_date == -1) {
			goto undo0;
		}
		if ((expires_date != 0) && (data.feed.download_date < expires_date)) {
			INFO("Content hasn't expired yet - aborting update without error.");
			success = true;
			goto undo0;
		}
	}

	data.feed.http_header_last_modified = db_get_date_from_feeds_table(url, "http_header_last_modified", 25);
	if (data.feed.http_header_last_modified == -1) {
		// Error message written by db_get_date_from_feeds_table.
		goto undo0;
	}

	data.feed.http_header_etag = db_get_string_from_feed_table(url, "http_header_etag", 16);
	if (data.feed.http_header_etag == NULL) {
		// Error message written by db_get_string_from_feed_table.
		goto undo0;
	}

	enum download_status status = download_feed(url->ptr, &data);

	if (status == DOWNLOAD_CANCELED) {
		INFO("Download canceled.");
		success = true;
		goto undo1;
	} else if (status == DOWNLOAD_FAILED) {
		WARN("Download failed.");
		goto undo1;
	}

	success = insert_feed(url, &data.feed);

undo1:
	free_feed(&data.feed);
undo0:
	return success;
}
