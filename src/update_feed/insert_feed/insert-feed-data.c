#include "update_feed/insert_feed/insert_feed.h"

bool
insert_feed_data(const struct string *feed_url, struct getfeed_feed *feed)
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
	if (db_prepare("INSERT OR REPLACE INTO feeds VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", 85, &s, NULL) == false) {
		success = false;
		goto undo5;
	}
	db_bind_string(s,      1 + FEED_COLUMN_FEED_URL,                  feed_url);
	db_bind_text_struct(s, 1 + FEED_COLUMN_TITLE,                     &feed->title);
	db_bind_string(s,      1 + FEED_COLUMN_LINK,                      feed->url);
	db_bind_text_struct(s, 1 + FEED_COLUMN_SUMMARY,                   &feed->summary);
	db_bind_string(s,      1 + FEED_COLUMN_AUTHORS,                   authors_str);
	db_bind_string(s,      1 + FEED_COLUMN_EDITORS,                   editors_str);
	db_bind_string(s,      1 + FEED_COLUMN_WEBMASTERS,                webmasters_str);
	db_bind_string(s,      1 + FEED_COLUMN_CATEGORIES,                categories_str);
	db_bind_string(s,      1 + FEED_COLUMN_LANGUAGES,                 feed->language);
	db_bind_string(s,      1 + FEED_COLUMN_GENERATOR,                 generator_str);
	db_bind_text_struct(s, 1 + FEED_COLUMN_RIGHTS,                    &feed->rights);
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_UPDATE_DATE,               (sqlite3_int64)(feed->update_date));
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_DOWNLOAD_DATE,             (sqlite3_int64)(feed->download_date));
	db_bind_string(s,      1 + FEED_COLUMN_HTTP_HEADER_ETAG,          feed->http_header_etag);
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_HTTP_HEADER_LAST_MODIFIED, (sqlite3_int64)(feed->http_header_last_modified));
	sqlite3_bind_int64(s,  1 + FEED_COLUMN_HTTP_HEADER_EXPIRES,       (sqlite3_int64)(feed->http_header_expires));
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
