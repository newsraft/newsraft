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
db_set_item_int(int64_t rowid, const char *column, size_t column_len, int value)
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
db_mark_item_read(int64_t rowid, bool status)
{
	return db_set_item_int(rowid, "unread", 6, status == false ? 1 : 0);
}

bool
db_mark_item_important(int64_t rowid, bool status)
{
	return db_set_item_int(rowid, "important", 9, status == false ? 0 : 1);
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
		db_bind_string(res, 1, url);
		if (sqlite3_step(res) == SQLITE_ROW) {
			unread_count = sqlite3_column_int64(res, 0);
		}
		sqlite3_finalize(res);
	}

	return unread_count;
}

int64_t
db_count_items(struct feed_entry **feeds, size_t feeds_count, bool count_only_unread)
{
	char *query = malloc(sizeof(char) * (60 + feeds_count * 2 + 100));
	if (feeds_count == 0 || query == NULL) {
		free(query);
		return 0;
	}
	if (count_only_unread) {
		strcpy(query, "SELECT COUNT(*) FROM items WHERE unread=1 AND feed_url IN (?");
	} else {
		strcpy(query, "SELECT COUNT(*) FROM items WHERE feed_url IN (?");
	}
	for (size_t i = 1; i < feeds_count; ++i) {
		strcat(query, ",?");
	}
	strcat(query, ")");
	sqlite3_stmt *res = db_prepare(query, strlen(query) + 1);
	if (res == NULL) {
		free(query);
		return 0;
	}
	for (size_t i = 0; i < feeds_count; ++i) {
		db_bind_string(res, i + 1, feeds[i]->link);
	}
	int64_t count = 0;
	if (sqlite3_step(res) == SQLITE_ROW) {
		count = sqlite3_column_int64(res, 0);
	}
	sqlite3_finalize(res);
	free(query);
	return count > 0 ? count : 0;
}

bool
db_change_unread_status_of_all_items_in_feeds(struct feed_entry **feeds, size_t feeds_count, bool unread)
{
	char *query = malloc(sizeof(char) * (46 + feeds_count * 2 + 100));
	if (feeds_count == 0 || query == NULL) {
		free(query);
		return true;
	}
	strcpy(query, "UPDATE items SET unread=? WHERE feed_url IN (?");
	for (size_t i = 1; i < feeds_count; ++i) {
		strcat(query, ",?");
	}
	strcat(query, ")");
	sqlite3_stmt *res = db_prepare(query, strlen(query) + 1);
	if (res == NULL) {
		free(query);
		return false;
	}
	sqlite3_bind_int(res, 1, unread);
	for (size_t i = 0; i < feeds_count; ++i) {
		db_bind_string(res, i + 2, feeds[i]->link);
	}
	bool status = sqlite3_step(res) == SQLITE_DONE ? true : false;
	sqlite3_finalize(res);
	free(query);
	return status;
}
