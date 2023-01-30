#include <string.h>
#include "newsraft.h"

sqlite3_stmt *
db_find_item_by_rowid(int64_t rowid)
{
	INFO("Looking for item with rowid %" PRId64 "...", rowid);
	sqlite3_stmt *res = db_prepare("SELECT * FROM items WHERE rowid=? LIMIT 1", 42);
	if (res == NULL) {
		return NULL;
	}
	sqlite3_bind_int64(res, 1, rowid);
	if (sqlite3_step(res) != SQLITE_ROW) {
		WARN("Item with rowid %" PRId64 " is not found!", rowid);
		sqlite3_finalize(res);
		return NULL;
	}
	INFO("Item with rowid %" PRId64 " is found.", rowid);
	return res;
}

static inline bool
db_update_item_int(int64_t rowid, const char *column, size_t column_len, int value)
{
	INFO("Updating column \"%s\" with integer \"%d\" of item with rowid \"%" PRId64 "\".", column, value, rowid);
	char cmd[100];
	memcpy(cmd, "UPDATE items SET ", 17);
	memcpy(cmd + 17, column, column_len);
	memcpy(cmd + 17 + column_len, "=? WHERE rowid=?", 17);
	sqlite3_stmt *res = db_prepare(cmd, 17 + column_len + 17);
	if (res != NULL) {
		sqlite3_bind_int(res, 1, value);
		sqlite3_bind_int64(res, 2, rowid);
		if (sqlite3_step(res) == SQLITE_DONE) {
			sqlite3_finalize(res);
			return true;
		}
		sqlite3_finalize(res);
	}
	FAIL("Column wasn't updated for some reason!");
	return false;
}

bool
db_mark_item_read(int64_t rowid)
{
	return db_update_item_int(rowid, "unread", 6, 0);
}

bool
db_mark_item_unread(int64_t rowid)
{
	return db_update_item_int(rowid, "unread", 6, 1);
}

bool
db_mark_item_important(int64_t rowid)
{
	return db_update_item_int(rowid, "important", 9, 1);
}

bool
db_mark_item_unimportant(int64_t rowid)
{
	return db_update_item_int(rowid, "important", 9, 0);
}

int64_t
get_unread_items_count_of_the_feed(const struct string *url)
{
	INFO("Counting unread items of the \"%s\" feed.", url->ptr);

	// This variable will be rewritten with proper value if everything goes okay;
	// otherwise it will remain negative to indicate an error.
	int64_t unread_count = -1;

	sqlite3_stmt *res = db_prepare("SELECT COUNT(*) FROM items WHERE feed_url=? AND unread=1", 57);
	if (res != NULL) {
		sqlite3_bind_text(res, 1, url->ptr, url->len, SQLITE_STATIC);
		if (sqlite3_step(res) == SQLITE_ROW) {
			unread_count = sqlite3_column_int64(res, 0);
		}
		sqlite3_finalize(res);
	}

	return unread_count;
}

static bool
change_unread_status_of_all_items_in_feeds(struct feed_entry **feeds, size_t feeds_count, bool unread)
{
	if (feeds_count == 0) {
		return true;
	}
	struct string *query = crtas("UPDATE items SET unread=? WHERE feed_url IN (?", 46);
	if (query == NULL) {
		return false;
	}
	for (size_t i = 1; i < feeds_count; ++i) {
		if (catas(query, ",?", 2) == false) {
			free_string(query);
			return false;
		}
	}
	if (catcs(query, ')') == false) {
		free_string(query);
		return false;
	}
	sqlite3_stmt *res = db_prepare(query->ptr, query->len + 1);
	if (res == NULL) {
		free_string(query);
		return false;
	}
	sqlite3_bind_int(res, 1, unread);
	for (size_t i = 0; i < feeds_count; ++i) {
		sqlite3_bind_text(res, i + 2, feeds[i]->link->ptr, feeds[i]->link->len, SQLITE_STATIC);
	}
	if (sqlite3_step(res) != SQLITE_DONE) {
		sqlite3_finalize(res);
		free_string(query);
		return false;
	}
	sqlite3_finalize(res);
	free_string(query);
	return true;
}

bool
db_mark_all_items_in_feeds_as_read(struct feed_entry **feeds, size_t feeds_count)
{
	return change_unread_status_of_all_items_in_feeds(feeds, feeds_count, false);
}

bool
db_mark_all_items_in_feeds_as_unread(struct feed_entry **feeds, size_t feeds_count)
{
	return change_unread_status_of_all_items_in_feeds(feeds, feeds_count, true);
}
