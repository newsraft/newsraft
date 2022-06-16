#include "update_feed/insert_feed/insert_feed.h"

bool
insert_feed_data(const struct string *feed_url, struct getfeed_feed *feed)
{
	sqlite3_stmt *s;
	if (db_prepare("INSERT OR REPLACE INTO feeds VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", 91, &s) == false) {
		return false;
	}
	db_bind_string(s,      1 + FEED_COLUMN_FEED_URL,                  feed_url);
	db_bind_string(s,      1 + FEED_COLUMN_GUID,                      feed->guid);
	db_bind_text_struct(s, 1 + FEED_COLUMN_TITLE,                     &feed->title);
	db_bind_string(s,      1 + FEED_COLUMN_LINK,                      feed->url);
	db_bind_text_struct(s, 1 + FEED_COLUMN_SUMMARY,                   &feed->summary);
	db_bind_string(s,      1 + FEED_COLUMN_PERSONS,                   feed->persons);
	db_bind_string(s,      1 + FEED_COLUMN_CATEGORIES,                feed->categories);
	db_bind_string(s,      1 + FEED_COLUMN_LANGUAGES,                 feed->language);
	db_bind_text_struct(s, 1 + FEED_COLUMN_RIGHTS,                    &feed->rights);
	db_bind_string(s,      1 + FEED_COLUMN_RATING,                    feed->rating);
	db_bind_string(s,      1 + FEED_COLUMN_PICTURES,                  feed->pictures);
	db_bind_string(s,      1 + FEED_COLUMN_GENERATORS,                feed->generators);
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_DOWNLOAD_DATE,             (sqlite3_int64)(feed->download_date));
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_UPDATE_DATE,               (sqlite3_int64)(feed->update_date));
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_TIME_TO_LIVE,              (sqlite3_int64)(feed->time_to_live));
	db_bind_string(s,      1 + FEED_COLUMN_HTTP_HEADER_ETAG,          feed->http_header_etag);
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_HTTP_HEADER_LAST_MODIFIED, (sqlite3_int64)(feed->http_header_last_modified));
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_HTTP_HEADER_EXPIRES,       (sqlite3_int64)(feed->http_header_expires));
	if (sqlite3_step(s) != SQLITE_DONE) {
		FAIL("Failed to insert feed data: %s", db_error_string());
		sqlite3_finalize(s);
		return false;
	}
	sqlite3_finalize(s);
	return true;
}
