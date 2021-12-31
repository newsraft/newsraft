#include "feedeater.h"
#include "update_feed.h"

bool
insert_feed(const struct string *feed_url, struct feed_bucket *feed)
{
	sqlite3_stmt *s;
	if (db_prepare("INSERT OR REPLACE INTO feeds VALUES(?, ?, ?, ?, ?, ?, ?);", 58, &s, NULL) != SQLITE_OK) {
		return false;
	}
	sqlite3_bind_text(s, FEED_COLUMN_FEED_URL     + 1, feed_url->ptr,           feed_url->len,           NULL);
	sqlite3_bind_text(s, FEED_COLUMN_TITLE        + 1, feed->title->ptr,        feed->title->len,        NULL);
	sqlite3_bind_text(s, FEED_COLUMN_LINK         + 1, feed->link->ptr,         feed->link->len,         NULL);
	sqlite3_bind_text(s, FEED_COLUMN_AUTHORS      + 1, "",                      0,                       NULL);
	sqlite3_bind_text(s, FEED_COLUMN_CATEGORIES   + 1, "",                      0,                       NULL);
	sqlite3_bind_text(s, FEED_COLUMN_SUMMARY      + 1, feed->summary->ptr,      feed->summary->len,      NULL);
	sqlite3_bind_text(s, FEED_COLUMN_SUMMARY_TYPE + 1, feed->summary_type->ptr, feed->summary_type->len, NULL);
	if (sqlite3_step(s) == SQLITE_DONE) {
		sqlite3_finalize(s);
		return true;
	}
	FAIL("Insertion of feed data failed: %s", db_error_string());
	sqlite3_finalize(s);
	return false;
}
