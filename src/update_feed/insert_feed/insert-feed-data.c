#include "update_feed/insert_feed/insert_feed.h"

bool
insert_feed_data(const struct string *feed_url, struct getfeed_feed *feed)
{
	bool success = false;
	struct string *generator_str = generate_generator_string(&feed->generator);
	if (generator_str == NULL) {
		goto undo0;
	}
	sqlite3_stmt *s;
	if (db_prepare("INSERT OR REPLACE INTO feeds VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", 91, &s) == false) {
		goto undo1;
	}
	db_bind_string(s,      1 + FEED_COLUMN_FEED_URL,                  feed_url);
	db_bind_string(s,      1 + FEED_COLUMN_GUID,                      feed->guid);
	db_bind_text_struct(s, 1 + FEED_COLUMN_TITLE,                     &feed->title);
	db_bind_string(s,      1 + FEED_COLUMN_LINK,                      feed->url);
	db_bind_text_struct(s, 1 + FEED_COLUMN_SUMMARY,                   &feed->summary);
	db_bind_string(s,      1 + FEED_COLUMN_AUTHORS,                   feed->authors);
	db_bind_string(s,      1 + FEED_COLUMN_CATEGORIES,                feed->categories);
	db_bind_string(s,      1 + FEED_COLUMN_LANGUAGES,                 feed->language);
	db_bind_text_struct(s, 1 + FEED_COLUMN_RIGHTS,                    &feed->rights);
	db_bind_string(s,      1 + FEED_COLUMN_RATING,                    feed->rating);
	db_bind_string(s,      1 + FEED_COLUMN_PICTURES,                  feed->pictures);
	db_bind_string(s,      1 + FEED_COLUMN_GENERATORS,                generator_str);
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_DOWNLOAD_DATE,             (sqlite3_int64)(feed->download_date));
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_UPDATE_DATE,               (sqlite3_int64)(feed->update_date));
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_TIME_TO_LIVE,              (sqlite3_int64)(feed->time_to_live));
	db_bind_string(s,      1 + FEED_COLUMN_HTTP_HEADER_ETAG,          feed->http_header_etag);
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_HTTP_HEADER_LAST_MODIFIED, (sqlite3_int64)(feed->http_header_last_modified));
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_HTTP_HEADER_EXPIRES,       (sqlite3_int64)(feed->http_header_expires));
	if (sqlite3_step(s) == SQLITE_DONE) {
		success = true;
	} else {
		FAIL("Insertion of feed data failed: %s", db_error_string());
	}
	sqlite3_finalize(s);
undo1:
	free_string(generator_str);
undo0:
	return success;
}
