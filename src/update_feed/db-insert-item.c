#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

static void
delete_excess_items(const struct string *feed_url) {
	sqlite3_stmt *s;
	if (sqlite3_prepare_v2(db, "SELECT rowid FROM items WHERE feed = ? ORDER BY upddate DESC, pubdate DESC, rowid DESC", -1, &s, 0) != SQLITE_OK) {
		FAIL("Failed to prepare an excess items deletion statement:");
		FAIL_SQLITE_PREPARE;
		return;
	}
	sqlite3_bind_text(s, 1, feed_url->ptr, feed_url->len, NULL);
	sqlite3_stmt *t;
	size_t item_iterator = 0;
	while (sqlite3_step(s) == SQLITE_ROW) {
		++item_iterator;
		if (item_iterator <= config_max_items) {
			continue;
		}
		if (sqlite3_prepare_v2(db, "DELETE FROM items WHERE rowid = ?", -1, &t, 0) == SQLITE_OK) {
			sqlite3_bind_int(t, 1, sqlite3_column_int(s, 0));
			if (sqlite3_step(t) == SQLITE_DONE) {
				INFO("Successfully deleted an item.");
			} else {
				FAIL("Deletion of the excess item is failed!");
			}
			sqlite3_finalize(t);
		} else {
			FAIL_SQLITE_PREPARE;
		}
	}
	sqlite3_finalize(s);
}

static bool
is_item_unique(const struct string *feed_url, const struct item_bucket *bucket)
{
	bool is_item_unique = false;
	sqlite3_stmt *res;
	if (bucket->guid->len > 0) { // check uniqueness by guid, upddate and pubdate
		char cmd[] = "SELECT * FROM items WHERE feed = ? AND guid = ? AND upddate = ? AND pubdate = ? LIMIT 1";
		if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
			sqlite3_bind_text(res,  1, feed_url->ptr, feed_url->len, NULL);
			sqlite3_bind_text(res,  2, bucket->guid->ptr, bucket->guid->len, NULL);
			sqlite3_bind_int64(res, 3, (sqlite3_int64)(bucket->upddate));
			sqlite3_bind_int64(res, 4, (sqlite3_int64)(bucket->pubdate));
			if (sqlite3_step(res) == SQLITE_DONE) {
				is_item_unique = true;
			}
			sqlite3_finalize(res);
		} else {
			FAIL_SQLITE_PREPARE;
		}
	} else { // check uniqueness by url, title, upddate and pubdate
		char cmd[] = "SELECT * FROM items WHERE feed = ? AND url = ? AND title = ? AND upddate = ? AND pubdate = ? LIMIT 1";
		if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
			sqlite3_bind_text(res,  1, feed_url->ptr, feed_url->len, NULL);
			sqlite3_bind_text(res,  2, bucket->url->ptr, bucket->url->len, NULL);
			sqlite3_bind_text(res,  3, bucket->title->ptr, bucket->title->len, NULL);
			sqlite3_bind_int64(res, 4, (sqlite3_int64)(bucket->upddate));
			sqlite3_bind_int64(res, 5, (sqlite3_int64)(bucket->pubdate));
			if (sqlite3_step(res) == SQLITE_DONE) {
				is_item_unique = true;
			}
			sqlite3_finalize(res);
		} else {
			FAIL_SQLITE_PREPARE;
		}
	}
	return is_item_unique;
}

static struct string *
create_authors_string(struct author *authors, size_t authors_count)
{
	struct string *authors_list = create_empty_string();
	if (authors_list == NULL) {
		FAIL("Not enough memory for creating authors string in item bucket!");
		return NULL;
	}
	bool added_name, added_email, added_link;
	for (size_t i = 0; i < authors_count; ++i) {
		added_name = false;
		added_email = false;
		added_link = false;
		if ((authors[i].name != NULL) && (authors[i].name->len != 0)) {
			catss(authors_list, authors[i].name);
			added_name = true;
		}
		if ((authors[i].email != NULL) && (authors[i].email->len != 0)) {
			if (added_name == true) {
				catas(authors_list, " <", 2);
			}
			catss(authors_list, authors[i].email);
			if (added_name == true) {
				catcs(authors_list, '>');
			}
			added_email = true;
		}
		if ((authors[i].link != NULL) && (authors[i].link->len != 0)) {
			if (added_name == true || added_email == true) {
				catas(authors_list, " (", 2);
			}
			catss(authors_list, authors[i].link);
			if (added_name == true || added_email == true) {
				catcs(authors_list, ')');
			}
			added_link = true;
		}
		if ((i + 1 != authors_count) &&
		    (added_name == true || added_email == true || added_link == true))
		{
			catas(authors_list, ", ", 2);
		}
	}
	return authors_list;
}

static void
db_insert_item(const struct string *feed_url, const struct item_bucket *bucket)
{
	struct string *authors_list = create_authors_string(bucket->authors, bucket->authors_count);
	if (authors_list == NULL) {
		return;
	}

	sqlite3_stmt *s;
	if (sqlite3_prepare_v2(db, "INSERT INTO items VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", -1, &s, 0) != SQLITE_OK) {
		FAIL("Failed to prepare item insertion statement:");
		FAIL_SQLITE_PREPARE;
		free_string(authors_list);
		return;
	}

	sqlite3_bind_text(s,  ITEM_COLUMN_FEED + 1,       feed_url->ptr, feed_url->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_TITLE + 1,      bucket->title->ptr, bucket->title->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_GUID + 1,       bucket->guid->ptr, bucket->guid->len, NULL);
	sqlite3_bind_int(s,   ITEM_COLUMN_UNREAD + 1,     1);
	sqlite3_bind_text(s,  ITEM_COLUMN_URL + 1,        bucket->url->ptr, bucket->url->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_AUTHORS + 1,    authors_list->ptr, authors_list->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_CATEGORIES + 1, bucket->categories->ptr, bucket->categories->len, NULL);
	sqlite3_bind_int64(s, ITEM_COLUMN_PUBDATE + 1,    (sqlite3_int64)(bucket->pubdate));
	sqlite3_bind_int64(s, ITEM_COLUMN_UPDDATE + 1,    (sqlite3_int64)(bucket->upddate));
	sqlite3_bind_text(s,  ITEM_COLUMN_COMMENTS + 1,   bucket->comments->ptr, bucket->comments->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_CONTENT + 1,    bucket->content->ptr, bucket->content->len, NULL);
	if (sqlite3_step(s) == SQLITE_DONE) {
		// only try to delete excess items if limit is set
		if (config_max_items != 0) {
			delete_excess_items(feed_url);
		}
	} else {
		FAIL("Item bucket insertion failed: %s", sqlite3_errmsg(db));
	}
	sqlite3_finalize(s);
	free_string(authors_list);
}

void
try_item_bucket(const struct item_bucket *bucket, const struct string *feed_url)
{
	if (is_item_unique(feed_url, bucket) == true) {
		db_insert_item(feed_url, bucket);
	} else {
		// TODO
		// db_update_item(feed_url, bucket);
	}
}
