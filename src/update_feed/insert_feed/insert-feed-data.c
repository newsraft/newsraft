#include "update_feed/insert_feed/insert_feed.h"

bool
insert_feed_data(const struct string *feed_url, const struct getfeed_feed *feed)
{
	bool success = true;
	struct string *authors_str = generate_person_list_string(feed->author);
	if (authors_str == NULL) {
		success = false;
		goto undo0;
	}
	struct string *editors_str = generate_person_list_string(feed->editor);
	if (editors_str == NULL) {
		success = false;
		goto undo1;
	}
	struct string *webmasters_str = generate_person_list_string(feed->webmaster);
	if (webmasters_str == NULL) {
		success = false;
		goto undo2;
	}
	struct string *generator_str = generate_generator_string(&feed->generator);
	if (generator_str == NULL) {
		success = false;
		goto undo3;
	}
	struct string *categories_str = generate_category_list_string(feed->category);
	if (categories_str == NULL) {
		success = false;
		goto undo4;
	}
	sqlite3_stmt *s;
	if (db_prepare("INSERT OR REPLACE INTO feeds VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", 82, &s, NULL) == false) {
		success = false;
		goto undo5;
	}
	sqlite3_bind_text(s,   FEED_COLUMN_FEED_URL                  + 1, feed_url->ptr,               feed_url->len,               NULL);
	db_bind_text_struct(s, FEED_COLUMN_TITLE                     + 1, &feed->title);
	sqlite3_bind_text(s,   FEED_COLUMN_LINK                      + 1, feed->url->ptr,              feed->url->len,              NULL);
	db_bind_text_struct(s, FEED_COLUMN_SUMMARY                   + 1, &feed->summary);
	sqlite3_bind_text(s,   FEED_COLUMN_AUTHORS                   + 1, authors_str->ptr,            authors_str->len,            NULL);
	sqlite3_bind_text(s,   FEED_COLUMN_EDITORS                   + 1, editors_str->ptr,            editors_str->len,            NULL);
	sqlite3_bind_text(s,   FEED_COLUMN_WEBMASTERS                + 1, webmasters_str->ptr,         webmasters_str->len,         NULL);
	sqlite3_bind_text(s,   FEED_COLUMN_CATEGORIES                + 1, categories_str->ptr,         categories_str->len,         NULL);
	sqlite3_bind_text(s,   FEED_COLUMN_LANGUAGE                  + 1, feed->language->ptr,         feed->language->len,         NULL);
	sqlite3_bind_text(s,   FEED_COLUMN_GENERATOR                 + 1, generator_str->ptr,          generator_str->len,          NULL);
	db_bind_text_struct(s, FEED_COLUMN_RIGHTS                    + 1, &feed->rights);
	sqlite3_bind_int64(s,  FEED_COLUMN_UPDATE_DATE               + 1, (sqlite3_int64)(feed->update_date));
	sqlite3_bind_int64(s,  FEED_COLUMN_DOWNLOAD_DATE             + 1, (sqlite3_int64)(feed->download_date));
	sqlite3_bind_text(s,   FEED_COLUMN_HTTP_HEADER_ETAG          + 1, feed->http_header_etag->ptr, feed->http_header_etag->len, NULL);
	sqlite3_bind_int64(s,  FEED_COLUMN_HTTP_HEADER_LAST_MODIFIED + 1, (sqlite3_int64)(feed->http_header_last_modified));
	if (sqlite3_step(s) != SQLITE_DONE) {
		FAIL("Insertion of feed data failed: %s", db_error_string());
		success = false;
	}
	sqlite3_finalize(s);
undo5:
	free_string(categories_str);
undo4:
	free_string(generator_str);
undo3:
	free_string(webmasters_str);
undo2:
	free_string(editors_str);
undo1:
	free_string(authors_str);
undo0:
	return success;
}
