#include <string.h>
#include "update_feed/insert_feed/insert_feed.h"

static inline bool
fnv_1a_string(struct string **target, const char *src)
{
	// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
	// http://isthe.com/chongo/tech/comp/fnv
	// https://datatracker.ietf.org/doc/html/draft-eastlake-fnv-17.html
	uint64_t hash = 14695981039346656037LLU;
	for (const char *i = src; *i != '\0'; ++i) {
		hash = (hash ^ *i) * 1099511628211LLU;
	}
	char str[60];
	size_t str_len = 47 + hash % 13, k = 0;
	while (k < str_len) {
		str[k] = hash % 94 + 33;
		hash = (hash ^ str[k++]) * 109951183333LLU;
	}
	str[str_len] = '\0';
	return crtas_or_cpyas(target, str, str_len);
}

bool
delete_excess_items(const struct string *feed_url, int64_t limit)
{
	INFO("Deleting excess items...");
	sqlite3_stmt *s = db_prepare("DELETE FROM items WHERE rowid IN (SELECT rowid FROM items WHERE feed_url=? ORDER BY unread DESC, publication_date DESC, update_date DESC, rowid DESC LIMIT -1 OFFSET ?) AND important=0", 184);
	if (s == NULL) {
		FAIL("Failed to prepare an excess items deletion statement!");
		return false;
	}
	db_bind_string(s, 1, feed_url);
	sqlite3_bind_int64(s, 2, limit);
	sqlite3_step(s);
	sqlite3_finalize(s);
	return true;
}

static inline bool
db_insert_item(const struct string *feed_url, struct getfeed_item *item, int64_t rowid)
{
	sqlite3_stmt *s;

	if (rowid == -1) {
		s = db_prepare("INSERT INTO items(feed_url,guid,title,link,content,attachments,persons,extras,publication_date,update_date,unread) VALUES(?,?,?,?,?,?,?,?,?,?,?)", 145);
	} else {
		s = db_prepare("UPDATE items SET feed_url=?,guid=?,title=?,link=?,content=?,attachments=?,persons=?,extras=?,publication_date=?,update_date=?,unread=? WHERE rowid=?", 149);
	}
	if (s == NULL) {
		FAIL("Failed to prepare item insertion/update statement!");
		return false;
	}

	db_bind_string(s,     1 + ITEM_COLUMN_FEED_URL,         feed_url);
	db_bind_string(s,     1 + ITEM_COLUMN_GUID,             item->guid);
	db_bind_string(s,     1 + ITEM_COLUMN_TITLE,            item->title);
	db_bind_string(s,     1 + ITEM_COLUMN_LINK,             item->url);
	db_bind_string(s,     1 + ITEM_COLUMN_CONTENT,          item->content);
	db_bind_string(s,     1 + ITEM_COLUMN_ATTACHMENTS,      item->attachments);
	db_bind_string(s,     1 + ITEM_COLUMN_PERSONS,          item->persons);
	db_bind_string(s,     1 + ITEM_COLUMN_EXTRAS,           item->extras);
	sqlite3_bind_int64(s, 1 + ITEM_COLUMN_PUBLICATION_DATE, item->publication_date);
	sqlite3_bind_int64(s, 1 + ITEM_COLUMN_UPDATE_DATE,      item->update_date);
	sqlite3_bind_int(s,   1 + ITEM_COLUMN_UNREAD,           1);
	if (rowid != -1) {
		sqlite3_bind_int64(s, 2 + ITEM_COLUMN_UNREAD, rowid);
	}

	if (sqlite3_step(s) != SQLITE_DONE) {
		if (rowid == -1) {
			FAIL("Item insertion failed: %s", db_error_string());
		} else {
			FAIL("Item update failed: %s", db_error_string());
		}
		sqlite3_finalize(s);
		return false;
	}

	sqlite3_finalize(s);

	return true;
}

bool
insert_item_data(const struct string *feed_url, struct getfeed_item *item)
{
	// Create guid if it was not set.
	if ((item->guid == NULL) || (item->guid->len == 0)) {
		if ((item->url != NULL) && (item->url->len != 0)) {
			if (crtss_or_cpyss(&item->guid, item->url) == false) {
				return false;
			}
		} else if ((item->title != NULL) && (item->title->len != 0)) {
			if (crtss_or_cpyss(&item->guid, item->title) == false) {
				return false;
			}
		} else if ((item->content != NULL) && (item->content->len != 0)) {
			if (fnv_1a_string(&item->guid, item->content->ptr) == false) {
				return false;
			}
		} else {
			WARN("Couldn't generate GUID for the item!");
			return true; // Probably this item is just empty. Ignore it.
		}
	}

	// Before trying to write some item to the database we have to check if this
	// item is duplicate or not.
	sqlite3_stmt *s = db_prepare("SELECT rowid,content FROM items WHERE feed_url=? AND guid=? LIMIT 1", 68);
	if (s == NULL) {
		FAIL("Failed to prepare SELECT statement for searching item duplicate by guid!");
		return false;
	}
	db_bind_string(s, 1, feed_url);
	db_bind_string(s, 2, item->guid);

	int64_t item_rowid;
	if (sqlite3_step(s) == SQLITE_ROW) {
		const char *content = (char *)sqlite3_column_text(s, 1);
		if (((content == NULL) && (item->content == NULL))
			|| ((content != NULL) && (item->content != NULL)
				&& (strcmp(content, item->content->ptr) == 0)))
		{
			sqlite3_finalize(s);
			return true;
		}
		item_rowid = sqlite3_column_int64(s, 0);
	} else {
		item_rowid = -1;
	}

	sqlite3_finalize(s);

	return db_insert_item(feed_url, item, item_rowid);
}
