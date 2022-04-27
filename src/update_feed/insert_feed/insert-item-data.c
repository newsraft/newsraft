#include <string.h>
#include "update_feed/insert_feed/insert_feed.h"

void
delete_excess_items(const struct string *feed_url)
{
	INFO("Deleting excess items...");
	sqlite3_stmt *s;
	if (db_prepare("SELECT rowid FROM items WHERE feed_url = ? ORDER BY upddate DESC, pubdate DESC, rowid DESC;", 92, &s, NULL) == false) {
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
		if (db_prepare("DELETE FROM items WHERE rowid = ?", 34, &t, NULL) == true) {
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
db_insert_item(const struct string *feed_url, const struct getfeed_item *item, int rowid)
{
	struct string *authors_list = generate_person_list_string(item->author);
	if (authors_list == NULL) {
		FAIL("Not enough memory for creating authors list of item bucket!");
		return;
	}

	struct string *attachments_list = generate_link_list_string(item->attachment);
	if (attachments_list == NULL) {
		FAIL("Not enough memory for creating attachments list of item bucket!");
		free_string(authors_list);
		return;
	}

	struct string *categories_list = generate_category_list_string(item->category);
	if (categories_list == NULL) {
		FAIL("Not enough memory for creating categories list of item bucket!");
		free_string(attachments_list);
		free_string(authors_list);
		return;
	}

	sqlite3_stmt *s;
	int prepare_status;

	if (rowid == -1) {
		prepare_status = db_prepare("INSERT INTO items VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", 66, &s, NULL);
	} else {
		prepare_status = db_prepare("UPDATE items SET feed_url = ?, title = ?, guid = ?, link = ?, unread = ?, attachments = ?, authors = ?, categories = ?, pubdate = ?, upddate = ?, comments_url = ?, summary = ?, content = ? WHERE rowid = ?;", 206, &s, NULL);
	}

	if (prepare_status == false) {
		FAIL("Failed to prepare item insertion statement!");
		free_string(categories_list);
		free_string(attachments_list);
		free_string(authors_list);
		return;
	}

	sqlite3_bind_text(s,   ITEM_COLUMN_FEED_URL     + 1, feed_url->ptr, feed_url->len, NULL);
	db_bind_text_struct(s, ITEM_COLUMN_TITLE        + 1, &item->title);
	sqlite3_bind_text(s,   ITEM_COLUMN_GUID         + 1, item->guid->ptr, item->guid->len, NULL);
	sqlite3_bind_text(s,   ITEM_COLUMN_LINK         + 1, item->url->ptr, item->url->len, NULL);
	sqlite3_bind_int(s,    ITEM_COLUMN_UNREAD       + 1, 1);
	sqlite3_bind_text(s,   ITEM_COLUMN_ATTACHMENTS  + 1, attachments_list->ptr, attachments_list->len, NULL);
	sqlite3_bind_text(s,   ITEM_COLUMN_AUTHORS      + 1, authors_list->ptr, authors_list->len, NULL);
	sqlite3_bind_text(s,   ITEM_COLUMN_CATEGORIES   + 1, categories_list->ptr, categories_list->len, NULL);
	sqlite3_bind_int64(s,  ITEM_COLUMN_PUBDATE      + 1, (sqlite3_int64)(item->pubdate));
	sqlite3_bind_int64(s,  ITEM_COLUMN_UPDDATE      + 1, (sqlite3_int64)(item->upddate));
	sqlite3_bind_text(s,   ITEM_COLUMN_COMMENTS_URL + 1, item->comments_url->ptr, item->comments_url->len, NULL);
	db_bind_text_struct(s, ITEM_COLUMN_SUMMARY      + 1, &item->summary);
	db_bind_text_struct(s, ITEM_COLUMN_CONTENT      + 1, &item->content);
	if (rowid != -1) {
		sqlite3_bind_int(s, ITEM_COLUMN_CONTENT + 2, rowid);
	}

	if (sqlite3_step(s) != SQLITE_DONE) {
		if (rowid == -1) {
			FAIL("Item insertion failed: %s", db_error_string());
		} else {
			FAIL("Item update failed: %s", db_error_string());
		}
	}

	sqlite3_finalize(s);
	free_string(categories_list);
	free_string(attachments_list);
	free_string(authors_list);
}

void
insert_item_data(const struct string *feed_url, const struct getfeed_item *item)
{
	sqlite3_stmt *s = NULL;
	int step_status;

	// Before trying to write some item to the database we have to
	// check if this item is duplicate or not.
	if (item->guid->len > 0) {
		// Most convenient way of verifying item uniqueness is to check
		// its unique ID.
		if (db_prepare("SELECT rowid, pubdate, upddate, content FROM items WHERE feed_url = ? AND guid = ? LIMIT 1;", 92, &s, NULL) == true) {
			sqlite3_bind_text(s, 1, feed_url->ptr, feed_url->len, NULL);
			sqlite3_bind_text(s, 2, item->guid->ptr, item->guid->len, NULL);
			step_status = sqlite3_step(s);
		} else {
			FAIL("Failed to prepare SELECT statement for searching item duplicate by guid!");
			return;
		}
	} else if (item->url->len > 0) {
		// Unique IDs are cool but not every feed format requires these IDs
		// to be set so all we can do here is to check uniqueness by some other
		// value and I think that link is the most suitable for that.
		if (db_prepare("SELECT rowid, pubdate, upddate, content FROM items WHERE feed_url = ? AND link = ? LIMIT 1;", 92, &s, NULL) == true) {
			sqlite3_bind_text(s, 1, feed_url->ptr, feed_url->len, NULL);
			sqlite3_bind_text(s, 2, item->url->ptr, item->url->len, NULL);
			step_status = sqlite3_step(s);
		} else {
			FAIL("Failed to prepare SELECT statement for searching item duplicate by link!");
			return;
		}
	} else {
		step_status = SQLITE_DONE;
	}

	int item_rowid = -1;
	if (step_status == SQLITE_ROW) {
		item_rowid = sqlite3_column_int(s, 0);
		int item_pubdate = sqlite3_column_int(s, 1);
		int item_upddate = sqlite3_column_int(s, 2);
		const char *item_content = (const char *)sqlite3_column_text(s, 3);
		const char *type_separator = strchr(item_content, ';');
		if (type_separator != NULL) {
			item_content = type_separator + 1;
		}
		if ((item_upddate == item->upddate) && (item_pubdate == item->pubdate) && (strcmp(item_content, item->content.value->ptr) == 0)) {
			// This is a complete duplicate of the item in database.
			sqlite3_finalize(s);
			return;
		}
	}

	if (s != NULL) {
		sqlite3_finalize(s);
	}

	db_insert_item(feed_url, item, item_rowid);
}
