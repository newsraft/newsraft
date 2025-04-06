#include <string.h>
#include "newsraft.h"

sqlite3_stmt *
db_find_item_by_rowid(int64_t rowid)
{
	INFO("Looking for item with rowid %" PRId64 "...", rowid);
	sqlite3_stmt *res = db_prepare("SELECT * FROM items WHERE rowid=? LIMIT 1", 42, NULL);
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
	INFO("Updating column \"%s\" with \"%d\" for item with rowid \"%" PRId64 "\".", column, value, rowid);
	char cmd[100] = {0};
	memcpy(cmd, "UPDATE items SET ", 17);
	memcpy(cmd + 17, column, column_len);
	memcpy(cmd + 17 + column_len, "=? WHERE rowid=?", 17);
	sqlite3_stmt *res = db_prepare(cmd, 17 + column_len + 17, NULL);
	if (res == NULL) {
		return false;
	}
	sqlite3_bind_int(res, 1, value);
	sqlite3_bind_int64(res, 2, rowid);
	int status = sqlite3_step(res);
	sqlite3_finalize(res);
	return status == SQLITE_DONE;
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
db_count_items(struct feed_entry **feeds, size_t feeds_count, bool count_only_unread)
{
	int64_t count = 0;
	struct string *query = crtas("SELECT COUNT(*) FROM items WHERE ", 33);
	struct string *cond = generate_items_search_condition(feeds, feeds_count);
	if (feeds_count == 0 || query == NULL || cond == NULL) {
		goto error;
	}
	catss(query, cond);
	if (count_only_unread) {
		catas(query, " AND unread=1", 13);
	}
	const char *error = "unknown error";
	sqlite3_stmt *res = db_prepare(query->ptr, query->len + 1, &error);
	if (res == NULL) {
		if (feeds_count == 1) {
			str_appendf((*feeds)->errors, "Failed to prepare statement, %s: %s\n", error, query->ptr);
			(*feeds)->has_errors = true;
		}
		goto error;
	}
	for (size_t i = 0; i < feeds_count; ++i) {
		db_bind_string(res, i + 1, feeds[i]->link);
	}
	if (sqlite3_step(res) == SQLITE_ROW) {
		count = sqlite3_column_int64(res, 0);
	} else {
		FAIL("sqlite3_step() in db_count_items() has failed!");
	}
	sqlite3_finalize(res);
error:
	free_string(query);
	free_string(cond);
	return count > 0 ? count : 0;
}

bool
db_change_unread_status_of_all_items_in_feeds(struct feed_entry **feeds, size_t feeds_count, bool unread)
{
	if (feeds_count == 0) {
		return true;
	}
	char *query = newsraft_malloc(sizeof(char) * (46 + feeds_count * 2 + 100));
	strcpy(query, "UPDATE items SET unread=? WHERE feed_url IN (?");
	for (size_t i = 1; i < feeds_count; ++i) {
		strcat(query, ",?");
	}
	strcat(query, ")");
	sqlite3_stmt *res = db_prepare(query, strlen(query) + 1, NULL);
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
