#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

static inline void
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

static inline struct string *
create_authors_string(struct author *authors, size_t authors_len)
{
	struct string *authors_list = create_empty_string();
	if (authors_list == NULL) {
		return NULL;
	}
	bool added_name, added_email, added_link;
	for (size_t i = 0; i < authors_len; ++i) {
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
		if ((i + 1 != authors_len) &&
		    (added_name == true || added_email == true || added_link == true))
		{
			catas(authors_list, ", ", 2);
		}
	}
	return authors_list;
}

static inline struct string *
create_enclosures_string(struct link *enclosures, size_t enclosures_len)
{
	struct string *enclosures_list = create_empty_string();
	if (enclosures_list == NULL) {
		return NULL;
	}
	bool added_type;
	for (size_t i = 0; i < enclosures_len; ++i) {
		added_type = false;
		if ((enclosures[i].url != NULL) && (enclosures[i].url->len != 0)) {
			catcs(enclosures_list, '\n');
			catss(enclosures_list, enclosures[i].url);
		} else {
			continue;
		}
		if ((enclosures[i].type != NULL) && (enclosures[i].type->len != 0)) {
			catas(enclosures_list, " (", 2);
			catss(enclosures_list, enclosures[i].type);
			catcs(enclosures_list, ')');
			added_type = true;
		}
	}
	return enclosures_list;
}

static inline void
db_insert_item(const struct string *feed_url, const struct item_bucket *bucket, int rowid)
{
	struct string *authors_list = create_authors_string(bucket->authors, bucket->authors_len);
	if (authors_list == NULL) {
		FAIL("Not enough memory for creating authors list of item bucket!");
		return;
	}

	struct string *enclosures_list = create_enclosures_string(bucket->enclosures, bucket->enclosures_len);
	if (enclosures_list == NULL) {
		FAIL("Not enough memory for creating enclosures list of item bucket!");
		free_string(authors_list);
		return;
	}

	sqlite3_stmt *s;
	int prepare_status;

	if (rowid == -1) {
		prepare_status = sqlite3_prepare_v2(db, "INSERT INTO items VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", -1, &s, 0);
	} else {
		prepare_status = sqlite3_prepare_v2(db, "UPDATE items SET feed = ?, title = ?, guid = ?, unread = ?, url = ?, enclosures = ?, authors = ?, categories = ?, pubdate = ?, upddate = ?, comments = ?, summary = ?, summary_type = ?, content = ?, content_type = ? WHERE rowid = ?;", -1, &s, 0);
	}

	if (prepare_status != SQLITE_OK) {
		FAIL_SQLITE_PREPARE;
		free_string(enclosures_list);
		free_string(authors_list);
		return;
	}

	sqlite3_bind_text(s,  ITEM_COLUMN_FEED         + 1, feed_url->ptr, feed_url->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_TITLE        + 1, bucket->title->ptr, bucket->title->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_GUID         + 1, bucket->guid->ptr, bucket->guid->len, NULL);
	sqlite3_bind_int(s,   ITEM_COLUMN_UNREAD       + 1, 1);
	sqlite3_bind_text(s,  ITEM_COLUMN_URL          + 1, bucket->url->ptr, bucket->url->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_ENCLOSURES   + 1, enclosures_list->ptr, enclosures_list->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_AUTHORS      + 1, authors_list->ptr, authors_list->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_CATEGORIES   + 1, bucket->categories->ptr, bucket->categories->len, NULL);
	sqlite3_bind_int64(s, ITEM_COLUMN_PUBDATE      + 1, (sqlite3_int64)(bucket->pubdate));
	sqlite3_bind_int64(s, ITEM_COLUMN_UPDDATE      + 1, (sqlite3_int64)(bucket->upddate));
	sqlite3_bind_text(s,  ITEM_COLUMN_COMMENTS     + 1, bucket->comments->ptr, bucket->comments->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_SUMMARY      + 1, bucket->summary->ptr, bucket->summary->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_SUMMARY_TYPE + 1, bucket->summary_type->ptr, bucket->summary_type->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_CONTENT      + 1, bucket->content->ptr, bucket->content->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_CONTENT_TYPE + 1, bucket->content_type->ptr, bucket->content_type->len, NULL);
	if (rowid != -1) {
		sqlite3_bind_int(s, ITEM_COLUMN_CONTENT_TYPE + 2, rowid);
	}

	if (sqlite3_step(s) == SQLITE_DONE) {
		if ((rowid == -1) && (config_max_items != 0)) {
			// Try to delete excess items only if the limit is set.
			delete_excess_items(feed_url);
		}
	} else {
		if (rowid == -1) {
			FAIL("Item insertion failed: %s", sqlite3_errmsg(db));
		} else {
			FAIL("Item update failed: %s", sqlite3_errmsg(db));
		}
	}

	sqlite3_finalize(s);
	free_string(enclosures_list);
	free_string(authors_list);
}

void
try_item_bucket(const struct item_bucket *bucket, const struct string *feed_url)
{
	sqlite3_stmt *res;
	int step_status;

	// Before trying to write some item to the database we have to
	// check if this item is duplicate or not.
	if (bucket->guid->len > 0) {
		// Most convenient way of verifying item uniqueness is to check
		// its unique ID.
		char cmd[] = "SELECT rowid, pubdate, upddate, content FROM items WHERE feed = ? AND guid = ? LIMIT 1";
		if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
			sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
			sqlite3_bind_text(res, 2, bucket->guid->ptr, bucket->guid->len, NULL);
			step_status = sqlite3_step(res);
		} else {
			FAIL("Failed to prepare SELECT statement for searching item duplicate by guid!");
			FAIL_SQLITE_PREPARE;
			return;
		}
	} else {
		// Unique IDs are cool but not every feed format requires these IDs
		// to be set so all we can do here is to check uniqueness by some other
		// identifiers and I think that URL and title are good for this.
		char cmd[] = "SELECT rowid, pubdate, upddate, content FROM items WHERE feed = ? AND url = ? AND title = ? LIMIT 1";
		if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
			sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
			sqlite3_bind_text(res, 2, bucket->url->ptr, bucket->url->len, NULL);
			sqlite3_bind_text(res, 3, bucket->title->ptr, bucket->title->len, NULL);
			step_status = sqlite3_step(res);
		} else {
			FAIL("Failed to prepare SELECT statement for searching item duplicate by url and title!");
			FAIL_SQLITE_PREPARE;
			return;
		}
	}

	int item_rowid = -1;
	int item_pubdate;
	int item_upddate;
	const char *item_content;
	if (step_status == SQLITE_ROW) {
		item_rowid = sqlite3_column_int(res, 0);
		item_pubdate = sqlite3_column_int(res, 1);
		item_upddate = sqlite3_column_int(res, 2);
		item_content = (const char *)sqlite3_column_text(res, 3);
		if ((item_upddate == bucket->upddate) && (item_pubdate == bucket->pubdate) && (strcmp(item_content, bucket->content->ptr) == 0)) {
			// This is a complete duplicate of the item in database.
			sqlite3_finalize(res);
			return;
		}
	}

	db_insert_item(feed_url, bucket, item_rowid);

	sqlite3_finalize(res);
}
