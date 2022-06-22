#include "update_feed/update_feed.h"

static inline void
free_feed(struct getfeed_feed *feed)
{
	free_string(feed->title.value);
	free_string(feed->title.type);
	free_string(feed->url);
	free_string(feed->content);
	free_string(feed->persons);
	free_string(feed->categories);
	free_string(feed->locations);
	free_string(feed->http_header_etag);
	free_item(feed->item);
}

int8_t
update_feed(const struct string *url)
{
	int8_t status = DOWNLOAD_FAILED;
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
			status = DOWNLOAD_CANCELED;
			goto undo0;
		}
	}

	if (get_cfg_bool(CFG_RESPECT_TTL_ELEMENT) == true) {
		int64_t ttl = db_get_date_from_feeds_table(url, "time_to_live", 12);
		int64_t prev_download_date = db_get_date_from_feeds_table(url, "download_date", 13);
		if ((ttl == -1) || (prev_download_date == -1)) {
			goto undo0;
		}
		if ((ttl != 0) && (prev_download_date != 0) && ((prev_download_date + ttl) > data.feed.download_date)) {
			INFO("Content isn't dead yet - aborting update without error.");
			status = DOWNLOAD_CANCELED;
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

	status = download_feed(url->ptr, &data);

	if (status == DOWNLOAD_CANCELED) {
		INFO("Download canceled.");
		goto undo1;
	} else if (status == DOWNLOAD_FAILED) {
		WARN("Download failed.");
		goto undo1;
	}

	if (insert_feed(url, &data.feed) == false) {
		status = DOWNLOAD_FAILED;
	}

undo1:
	free_feed(&data.feed);
undo0:
	return status;
}
