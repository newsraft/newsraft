#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

void
delete_excess_items(const struct string *feed_url) {
	INFO("Deleting excess items...");
	sqlite3_stmt *s;
	if (db_prepare("SELECT rowid FROM items WHERE feed_url = ? ORDER BY upddate DESC, pubdate DESC, rowid DESC;", 92, &s, NULL) != SQLITE_OK) {
		FAIL("Failed to prepare an excess items deletion statement:");
		return;
	}
	sqlite3_bind_text(s, 1, feed_url->ptr, feed_url->len, NULL);
	sqlite3_stmt *t;
	size_t item_iterator = 0;
	while (sqlite3_step(s) == SQLITE_ROW) {
		++item_iterator;
		if (item_iterator <= cfg.max_items) {
			continue;
		}
		if (db_prepare("DELETE FROM items WHERE rowid = ?", 34, &t, NULL) == SQLITE_OK) {
			sqlite3_bind_int(t, 1, sqlite3_column_int(s, 0));
			if (sqlite3_step(t) != SQLITE_DONE) {
				FAIL("Deletion of the excess item is failed!");
			}
			sqlite3_finalize(t);
		}
	}
	sqlite3_finalize(s);
}

static inline void
db_insert_item(const struct string *feed_url, const struct item_bucket *item, int rowid)
{
	struct string *authors_list = generate_person_list_string(&item->authors);
	if (authors_list == NULL) {
		FAIL("Not enough memory for creating authors list of item bucket!");
		return;
	}

	struct string *enclosures_list = generate_link_list_string_for_database(&item->enclosures);
	if (enclosures_list == NULL) {
		FAIL("Not enough memory for creating enclosures list of item bucket!");
		free_string(authors_list);
		return;
	}

	sqlite3_stmt *s;
	int prepare_status;

	if (rowid == -1) {
		prepare_status = db_prepare("INSERT INTO items VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", 72, &s, NULL);
	} else {
		prepare_status = db_prepare("UPDATE items SET feed_url = ?, title = ?, guid = ?, link = ?, unread = ?, enclosures = ?, authors = ?, categories = ?, pubdate = ?, upddate = ?, comments_url = ?, summary = ?, summary_type = ?, content = ?, content_type = ? WHERE rowid = ?;", 237, &s, NULL);
	}

	if (prepare_status != SQLITE_OK) {
		free_string(enclosures_list);
		free_string(authors_list);
		return;
	}

	sqlite3_bind_text(s,  ITEM_COLUMN_FEED_URL     + 1, feed_url->ptr, feed_url->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_TITLE        + 1, item->title->ptr, item->title->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_GUID         + 1, item->guid->ptr, item->guid->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_LINK         + 1, item->url->ptr, item->url->len, NULL);
	sqlite3_bind_int(s,   ITEM_COLUMN_UNREAD       + 1, 1);
	sqlite3_bind_text(s,  ITEM_COLUMN_ENCLOSURES   + 1, enclosures_list->ptr, enclosures_list->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_AUTHORS      + 1, authors_list->ptr, authors_list->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_CATEGORIES   + 1, item->categories->ptr, item->categories->len, NULL);
	sqlite3_bind_int64(s, ITEM_COLUMN_PUBDATE      + 1, (sqlite3_int64)(item->pubdate));
	sqlite3_bind_int64(s, ITEM_COLUMN_UPDDATE      + 1, (sqlite3_int64)(item->upddate));
	sqlite3_bind_text(s,  ITEM_COLUMN_COMMENTS_URL + 1, item->comments_url->ptr, item->comments_url->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_SUMMARY      + 1, item->summary->ptr, item->summary->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_SUMMARY_TYPE + 1, item->summary_type->ptr, item->summary_type->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_CONTENT      + 1, item->content->ptr, item->content->len, NULL);
	sqlite3_bind_text(s,  ITEM_COLUMN_CONTENT_TYPE + 1, item->content_type->ptr, item->content_type->len, NULL);
	if (rowid != -1) {
		sqlite3_bind_int(s, ITEM_COLUMN_CONTENT_TYPE + 2, rowid);
	}

	if (sqlite3_step(s) != SQLITE_DONE) {
		if (rowid == -1) {
			FAIL("Item insertion failed: %s", db_error_string());
		} else {
			FAIL("Item update failed: %s", db_error_string());
		}
	}

	sqlite3_finalize(s);
	free_string(enclosures_list);
	free_string(authors_list);
}

void
insert_item(const struct string *feed_url, const struct item_bucket *item)
{
	sqlite3_stmt *res;
	int step_status;

	// Before trying to write some item to the database we have to
	// check if this item is duplicate or not.
	if (item->guid->len > 0) {
		// Most convenient way of verifying item uniqueness is to check
		// its unique ID.
		if (db_prepare("SELECT rowid, pubdate, upddate, content FROM items WHERE feed_url = ? AND guid = ? LIMIT 1;", 92, &res, NULL) == SQLITE_OK) {
			sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
			sqlite3_bind_text(res, 2, item->guid->ptr, item->guid->len, NULL);
			step_status = sqlite3_step(res);
		} else {
			FAIL("Failed to prepare SELECT statement for searching item duplicate by guid!");
			return;
		}
	} else {
		// Unique IDs are cool but not every feed format requires these IDs
		// to be set so all we can do here is to check uniqueness by some other
		// identifiers and I think that URL and title are good for this.
		if (db_prepare("SELECT rowid, pubdate, upddate, content FROM items WHERE feed_url = ? AND link = ? AND title = ? LIMIT 1;", 106, &res, NULL) == SQLITE_OK) {
			sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
			sqlite3_bind_text(res, 2, item->url->ptr, item->url->len, NULL);
			sqlite3_bind_text(res, 3, item->title->ptr, item->title->len, NULL);
			step_status = sqlite3_step(res);
		} else {
			FAIL("Failed to prepare SELECT statement for searching item duplicate by link and title!");
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
		if ((item_upddate == item->upddate) && (item_pubdate == item->pubdate) && (strcmp(item_content, item->content->ptr) == 0)) {
			// This is a complete duplicate of the item in database.
			sqlite3_finalize(res);
			return;
		}
	}

	db_insert_item(feed_url, item, item_rowid);

	sqlite3_finalize(res);
}
