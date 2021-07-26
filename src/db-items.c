#include <string.h>
#include "feedeater.h"
#include "config.h"

static void
delete_excess_items(struct string *feed_url) {
	if (config_max_items == 0) {
		return; // config_max_items is set to infinity, delete nothing
	}
	sqlite3_stmt *s;
	if (sqlite3_prepare_v2(db, "SELECT rowid FROM items WHERE feed = ? ORDER BY upddate DESC, pubdate DESC, rowid ASC", -1, &s, 0) == SQLITE_OK) {
		sqlite3_bind_text(s, 1, feed_url->ptr, feed_url->len, NULL);
		sqlite3_stmt *t;
		size_t item_id = 0;
		while (sqlite3_step(s) == SQLITE_ROW) {
			++item_id;
			if (item_id < config_max_items + 1) {
				continue;
			}
			if (sqlite3_prepare_v2(db, "DELETE FROM items WHERE rowid = ?", -1, &t, 0) == SQLITE_OK) {
				sqlite3_bind_int(t, 1, sqlite3_column_int(s, 0));
				if (sqlite3_step(t) != SQLITE_DONE) {
					debug_write(DBG_ERR, "deletion of excess item failed: " PREPARE_DELETE_FAIL, sqlite3_errmsg(db));
				}
				sqlite3_finalize(t);
			}
		}
		sqlite3_finalize(s);
	} else {
		debug_write(DBG_ERR, "search for excess items failed: " PREPARE_SELECT_FAIL, sqlite3_errmsg(db));
	}
}

static void
db_insert_item(struct item_bucket *bucket, struct string *feed_url)
{
	sqlite3_stmt *s;
	if (sqlite3_prepare_v2(db, "INSERT INTO items VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", -1, &s, 0) != SQLITE_OK) {
		debug_write(DBG_ERR, "item bucket insertion failed:\n");
		debug_write(DBG_ERR, PREPARE_INSERT_FAIL, sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_text(s,  ITEM_COLUMN_FEED + 1,     feed_url->ptr, feed_url->len, NULL);
	db_bind_string(s,     ITEM_COLUMN_TITLE + 1,    bucket->title);
	db_bind_string(s,     ITEM_COLUMN_GUID + 1,     bucket->guid);
	sqlite3_bind_int(s,   ITEM_COLUMN_UNREAD + 1,   1);
	sqlite3_bind_int(s,   ITEM_COLUMN_MARKED + 1,   0);
	db_bind_string(s,     ITEM_COLUMN_URL + 1,      bucket->url);
	db_bind_string(s,     ITEM_COLUMN_AUTHOR + 1,   bucket->author);
	db_bind_string(s,     ITEM_COLUMN_CATEGORY + 1, bucket->category);
	sqlite3_bind_int64(s, ITEM_COLUMN_PUBDATE + 1,  (sqlite3_int64)(bucket->pubdate));
	sqlite3_bind_int64(s, ITEM_COLUMN_UPDDATE + 1,  (sqlite3_int64)(bucket->upddate));
	db_bind_string(s,     ITEM_COLUMN_COMMENTS + 1, bucket->comments);
	db_bind_string(s,     ITEM_COLUMN_CONTENT + 1,  bucket->content);
	if (sqlite3_step(s) == SQLITE_DONE) {
		delete_excess_items(feed_url);
	} else {
		debug_write(DBG_ERR, "item bucket insertion failed: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(s);
}

void
try_item_bucket(struct item_bucket *bucket, struct string *feed_url)
{
	bool is_item_unique = false;
	sqlite3_stmt *res;
	if (bucket->guid->len > 0) { // check uniqueness by guid, upddate and pubdate
		char cmd[] = "SELECT * FROM items WHERE feed = ? AND guid = ? AND upddate = ? AND pubdate = ? LIMIT 1";
		if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
			db_bind_string(res, 1, feed_url);
			db_bind_string(res, 2, bucket->guid);
			sqlite3_bind_int64(res, 3, (sqlite3_int64)(bucket->upddate));
			sqlite3_bind_int64(res, 4, (sqlite3_int64)(bucket->pubdate));
			if (sqlite3_step(res) == SQLITE_DONE) is_item_unique = true;
			sqlite3_finalize(res);
		} else {
			debug_write(DBG_WARN, PREPARE_SELECT_FAIL, sqlite3_errmsg(db));
		}
	} else { // check uniqueness by url, title, upddate and pubdate
		char cmd[] = "SELECT * FROM items WHERE feed = ? AND url = ? AND title = ? AND upddate = ? AND pubdate = ? LIMIT 1";
		if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
			db_bind_string(res, 1, feed_url);
			db_bind_string(res, 2, bucket->url);
			db_bind_string(res, 3, bucket->title);
			sqlite3_bind_int64(res, 4, (sqlite3_int64)(bucket->upddate));
			sqlite3_bind_int64(res, 5, (sqlite3_int64)(bucket->pubdate));
			if (sqlite3_step(res) == SQLITE_DONE) is_item_unique = true;
			sqlite3_finalize(res);
		} else {
			debug_write(DBG_WARN, PREPARE_SELECT_FAIL, sqlite3_errmsg(db));
		}
	}
	if (is_item_unique == true) db_insert_item(bucket, feed_url);
}

int
db_update_item_int(struct string *feed_url, struct item_entry *item, const char *state, int value)
{
	int success = 0;
	if (item == NULL) return success;
	char cmd[100];
	sqlite3_stmt *res;
	strcpy(cmd, "UPDATE items SET ");
	strcat(cmd, state);
	// TODO: make WHERE statement more precise
	strcat(cmd, " = ? WHERE feed = ? AND guid = ? AND url = ?");
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_int(res, 1, value);
		sqlite3_bind_text(res, 2, feed_url->ptr, feed_url->len, NULL);
		sqlite3_bind_text(res, 3, item->guid->ptr, item->guid->len, NULL);
		sqlite3_bind_text(res, 4, item->url->ptr, item->url->len, NULL);
		if (sqlite3_step(res) == SQLITE_DONE) success = 1;
		sqlite3_finalize(res);
	} else {
		debug_write(DBG_WARN, PREPARE_UPDATE_FAIL, sqlite3_errmsg(db));
	}
	return success;
}
