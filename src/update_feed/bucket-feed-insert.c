#include "update_feed/update_feed.h"

bool
insert_feed(const struct string *feed_url, const struct feed_bucket *feed)
{
	struct string *generator_str = generate_generator_string_for_database(&feed->generator);
	if (generator_str == NULL) {
		return false;
	}
	sqlite3_stmt *s;
	if (db_prepare("INSERT OR REPLACE INTO feeds VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", 76, &s, NULL) == false) {
		free_string(generator_str);
		return false;
	}
	sqlite3_bind_text(s,   FEED_COLUMN_FEED_URL      + 1, feed_url->ptr,       feed_url->len,       NULL);
	db_bind_text_struct(s, FEED_COLUMN_TITLE         + 1, &feed->title);
	sqlite3_bind_text(s,   FEED_COLUMN_LINK          + 1, feed->link->ptr,     feed->link->len,     NULL);
	db_bind_text_struct(s, FEED_COLUMN_SUMMARY       + 1, &feed->summary);
	sqlite3_bind_text(s,   FEED_COLUMN_AUTHORS       + 1, "",                  0,                   NULL);
	sqlite3_bind_text(s,   FEED_COLUMN_EDITORS       + 1, "",                  0,                   NULL);
	sqlite3_bind_text(s,   FEED_COLUMN_WEBMASTERS    + 1, "",                  0,                   NULL);
	sqlite3_bind_text(s,   FEED_COLUMN_CATEGORIES    + 1, "",                  0,                   NULL);
	sqlite3_bind_text(s,   FEED_COLUMN_LANGUAGE      + 1, feed->language->ptr, feed->language->len, NULL);
	sqlite3_bind_text(s,   FEED_COLUMN_GENERATOR     + 1, generator_str->ptr,  generator_str->len,  NULL);
	db_bind_text_struct(s, FEED_COLUMN_RIGHTS        + 1, &feed->rights);
	sqlite3_bind_int64(s,  FEED_COLUMN_UPDATE_TIME   + 1, (sqlite3_int64)(feed->update_time));
	sqlite3_bind_int64(s,  FEED_COLUMN_DOWNLOAD_TIME + 1, (sqlite3_int64)(feed->download_time));
	if (sqlite3_step(s) != SQLITE_DONE) {
		FAIL("Insertion of feed data failed: %s", db_error_string());
		sqlite3_finalize(s);
		free_string(generator_str);
		return false;
	}
	sqlite3_finalize(s);
	free_string(generator_str);
	if (cfg.max_items != 0) {
		delete_excess_items(feed_url);
	}
	return true;
}
