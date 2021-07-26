#include <string.h>
#include "feedeater.h"

static bool
is_feed_stored(struct string *feed_url)
{
	bool stored = true;
	sqlite3_stmt *s;
	if (sqlite3_prepare_v2(db, "SELECT * FROM feeds WHERE url = ? LIMIT 1", -1, &s, 0) == SQLITE_OK) {
		sqlite3_bind_text(s, 1, feed_url->ptr, feed_url->len, NULL);
		if (sqlite3_step(s) == SQLITE_DONE) stored = false;
		sqlite3_finalize(s);
	} else {
		debug_write(DBG_WARN, PREPARE_SELECT_FAIL, sqlite3_errmsg(db));
	}
	return stored;
}

static bool
create_feed_entry(struct string *feed_url)
{
	if (is_feed_stored(feed_url) == true) return true;
	bool created = false;
	sqlite3_stmt *s;
	if (sqlite3_prepare_v2(db, "INSERT INTO feeds (url) VALUES(?)", -1, &s, 0) == SQLITE_OK) {
		sqlite3_bind_text(s, 1, feed_url->ptr, feed_url->len, NULL);
		if (sqlite3_step(s) == SQLITE_DONE) created = true;
		sqlite3_finalize(s);
	} else {
		debug_write(DBG_WARN, PREPARE_INSERT_FAIL, sqlite3_errmsg(db));
	}
	return created;
}

void
db_update_feed_text(struct string *feed_url, char *column, char *data, size_t data_len)
{
	if (create_feed_entry(feed_url) == false) {
		debug_write(DBG_ERR, "could not create feed entry in database!\n");
		return;
	}
	char *cmd = malloc(sizeof(char) * (37 + strlen(column)));
	if (cmd == NULL) return;
	sqlite3_stmt *res;
	// BEFORE CHANGING FORMAT STRING LO^OK ABOVE
	sprintf(cmd, "UPDATE feeds SET %s = ? WHERE url = ?", column);
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_text(res, 1, data, data_len, NULL);
		sqlite3_bind_text(res, 2, feed_url->ptr, feed_url->len, NULL);
		/*if (sqlite3_step(res) == SQLITE_DONE) success = 1;*/
		sqlite3_step(res);
		sqlite3_finalize(res);
	} else {
		debug_write(DBG_WARN, PREPARE_UPDATE_FAIL, sqlite3_errmsg(db));
	}
	free(cmd);
}

bool
is_feed_marked(struct string *url)
{
	if (url == NULL) return false;
	bool is_marked = false;
	sqlite3_stmt *res;
	char cmd[] = "SELECT * FROM items WHERE feed = ? AND marked = ? LIMIT 1";
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_text(res, 1, url->ptr, url->len, NULL);
		sqlite3_bind_int(res, 2, 1);
		// if something found (at least one item from feed with this url is marked), say feed is marked
		if (sqlite3_step(res) == SQLITE_ROW) is_marked = true;
		sqlite3_finalize(res);
	} else {
		debug_write(DBG_ERR, "failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
	}
	return is_marked;
}

bool
is_feed_unread(struct string *url)
{
	if (url == NULL) return false;
	bool is_unread = false;
	sqlite3_stmt *res;
	char cmd[] = "SELECT * FROM items WHERE feed = ? AND unread = ? LIMIT 1";
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_text(res, 1, url->ptr, url->len, NULL);
		sqlite3_bind_int(res, 2, 1);
		// if something found (at least one item from feed with this url is unread), say feed is unread
		if (sqlite3_step(res) == SQLITE_ROW) is_unread = true;
		sqlite3_finalize(res);
	} else {
		debug_write(DBG_ERR, "failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
	}
	return is_unread;
}
