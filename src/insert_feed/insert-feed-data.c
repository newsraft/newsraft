#include "insert_feed/insert_feed.h"

bool
insert_feed_data(const struct feed_entry *feed_entry, struct getfeed_feed *feed)
{
	sqlite3_stmt *s = db_prepare("INSERT OR REPLACE INTO feeds(feed_url,title,link,content,attachments,persons,extras,download_date,update_date,time_to_live,http_header_etag,http_header_last_modified,http_header_expires) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?)", 221, NULL);
	if (s == NULL) {
		return false;
	}
	db_bind_string(s,     1 + FEED_COLUMN_FEED_URL,                  feed_entry->link);
	db_bind_string(s,     1 + FEED_COLUMN_TITLE,                     feed->title);
	db_bind_string(s,     1 + FEED_COLUMN_LINK,                      feed->url);
	db_bind_string(s,     1 + FEED_COLUMN_CONTENT,                   feed->content);
	db_bind_string(s,     1 + FEED_COLUMN_ATTACHMENTS,               feed->attachments);
	db_bind_string(s,     1 + FEED_COLUMN_PERSONS,                   feed->persons);
	db_bind_string(s,     1 + FEED_COLUMN_EXTRAS,                    feed->extras);
	sqlite3_bind_int64(s, 1 + FEED_COLUMN_DOWNLOAD_DATE,             feed_entry->update_date);
	sqlite3_bind_int64(s, 1 + FEED_COLUMN_UPDATE_DATE,               feed_entry->update_date);
	sqlite3_bind_int64(s, 1 + FEED_COLUMN_TIME_TO_LIVE,              feed->time_to_live);
	db_bind_string(s,     1 + FEED_COLUMN_HTTP_HEADER_ETAG,          feed->http_header_etag);
	sqlite3_bind_int64(s, 1 + FEED_COLUMN_HTTP_HEADER_LAST_MODIFIED, feed->http_header_last_modified);
	sqlite3_bind_int64(s, 1 + FEED_COLUMN_HTTP_HEADER_EXPIRES,       feed->http_header_expires);
	if (sqlite3_step(s) != SQLITE_DONE) {
		FAIL("Failed to insert or replace feed data: %s", db_error_string());
		sqlite3_finalize(s);
		return false;
	}
	sqlite3_finalize(s);
	return true;
}
