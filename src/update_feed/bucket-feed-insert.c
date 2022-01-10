#include "feedeater.h"
#include "update_feed.h"

bool
insert_feed(const struct string *feed_url, const struct feed_bucket *feed)
{
	sqlite3_stmt *s;
	if (db_prepare("INSERT OR REPLACE INTO feeds VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", 76, &s, NULL) == false) {
		return false;
	}
	sqlite3_bind_text(s,   FEED_COLUMN_FEED_URL      + 1, feed_url->ptr,       feed_url->len,           NULL);
	db_bind_text_struct(s, FEED_COLUMN_TITLE         + 1, &feed->title);
	sqlite3_bind_text(s,   FEED_COLUMN_LINK          + 1, feed->link->ptr,     feed->link->len,         NULL);
	db_bind_text_struct(s, FEED_COLUMN_SUMMARY       + 1, &feed->summary);
	sqlite3_bind_text(s,   FEED_COLUMN_AUTHORS       + 1, "",                  0,                       NULL);
	sqlite3_bind_text(s,   FEED_COLUMN_EDITORS       + 1, "",                  0,                       NULL);
	sqlite3_bind_text(s,   FEED_COLUMN_WEBMASTERS    + 1, "",                  0,                       NULL);
	sqlite3_bind_text(s,   FEED_COLUMN_CATEGORIES    + 1, "",                  0,                       NULL);
	sqlite3_bind_text(s,   FEED_COLUMN_LANGUAGE      + 1, feed->language->ptr, feed->language->len,     NULL);
	db_bind_text_struct(s, FEED_COLUMN_GENERATOR     + 1, &feed->generator);
	db_bind_text_struct(s, FEED_COLUMN_RIGHTS        + 1, &feed->rights);
	sqlite3_bind_int64(s,  FEED_COLUMN_UPDATE_TIME   + 1, (sqlite3_int64)(feed->update_time));
	sqlite3_bind_int64(s,  FEED_COLUMN_DOWNLOAD_TIME + 1, (sqlite3_int64)(feed->download_time));
	if (sqlite3_step(s) != SQLITE_DONE) {
		FAIL("Insertion of feed data failed: %s", db_error_string());
		sqlite3_finalize(s);
		return false;
	}
	sqlite3_finalize(s);
	return true;
}
