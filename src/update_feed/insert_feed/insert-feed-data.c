#include "update_feed/insert_feed/insert_feed.h"

bool
insert_feed_data(const struct string *feed_url, struct getfeed_feed *feed)
{
	sqlite3_stmt *s = db_prepare("INSERT OR REPLACE INTO feeds VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?)", 63);
	if (s == NULL) {
		return false;
	}
	db_bind_string(s,      1 + FEED_COLUMN_FEED_URL,                  feed_url);
	db_bind_string(s,      1 + FEED_COLUMN_TITLE,                     feed->title);
	db_bind_string(s,      1 + FEED_COLUMN_LINK,                      feed->url);
	db_bind_string(s,      1 + FEED_COLUMN_CONTENT,                   feed->content);
	db_bind_string(s,      1 + FEED_COLUMN_ATTACHMENTS,               feed->attachments);
	db_bind_string(s,      1 + FEED_COLUMN_PERSONS,                   feed->persons);
	db_bind_string(s,      1 + FEED_COLUMN_EXTRAS,                    feed->extras);
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_DOWNLOAD_DATE,             (sqlite3_int64)(feed->download_date));
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_UPDATE_DATE,               (sqlite3_int64)(feed->update_date));
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_TIME_TO_LIVE,              (sqlite3_int64)(feed->time_to_live));
	db_bind_string(s,      1 + FEED_COLUMN_HTTP_HEADER_ETAG,          feed->http_header_etag);
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_HTTP_HEADER_LAST_MODIFIED, (sqlite3_int64)(feed->http_header_last_modified));
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_HTTP_HEADER_EXPIRES,       (sqlite3_int64)(feed->http_header_expires));
	if (sqlite3_step(s) != SQLITE_DONE) {
		FAIL("Failed to insert or replace feed data: %s", db_error_string());
		sqlite3_finalize(s);
		return false;
	}
	sqlite3_finalize(s);
	return true;
}
