#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

// On success returns pointer to sqlite3_stmt.
// On failure returns NULL.
sqlite3_stmt *
db_find_item_by_rowid(int rowid)
{
	INFO("Looking for item with rowid %d...", rowid);
	sqlite3_stmt *res;
	if (db_prepare("SELECT * FROM items WHERE rowid = ? LIMIT 1", 44, &res) == false) {
		return NULL;
	}
	sqlite3_bind_int(res, 1, rowid);
	if (sqlite3_step(res) != SQLITE_ROW) {
		WARN("Item with rowid %d not found!", rowid);
		sqlite3_finalize(res);
		return NULL;
	}
	INFO("Item with rowid %d is found.", rowid);
	return res;
}

static bool
db_update_item_int(int rowid, const char *column, int value)
{
	INFO("Updating column \"%s\" with integer value \"%d\" of item with rowid \"%d\".", column, value, rowid);

	// Size of this buffer depends on the size of strings below.
	size_t cmd_size = sizeof(char) * (17 + strlen(column) + 20 + 1);
	char *cmd = malloc(cmd_size);
	if (cmd == NULL) {
		FAIL("Not enough memory for updating that column!");
		return false;
	}
	strcpy(cmd, "UPDATE items SET ");
	strcat(cmd, column);
	strcat(cmd, " = ? WHERE rowid = ?");

	bool success = true;

	sqlite3_stmt *res;
	if (db_prepare(cmd, cmd_size, &res) == true) {
		sqlite3_bind_int(res, 1, value);
		sqlite3_bind_int(res, 2, rowid);
		if (sqlite3_step(res) != SQLITE_DONE) {
			WARN("Column wasn't updated for some reason!");
			success = false;
		}
		sqlite3_finalize(res);
	}

	free(cmd);

	return success;
}

bool
db_mark_item_read(int rowid)
{
	return db_update_item_int(rowid, "unread", 0);
}

bool
db_mark_item_unread(int rowid)
{
	return db_update_item_int(rowid, "unread", 1);
}

int64_t
get_unread_items_count_of_the_feed(const struct string *url)
{
	INFO("Counting unread items of the \"%s\" feed.", url->ptr);

	// This variable will be rewritten with proper value if everything goes okay;
	// otherwise it will remain negative to indicate an error.
	int64_t unread_count = -1;

	sqlite3_stmt *res;
	if (db_prepare("SELECT COUNT(*) FROM items WHERE feed_url=? AND unread=1;", 58, &res) == true) {
		sqlite3_bind_text(res, 1, url->ptr, url->len, NULL);
		if (sqlite3_step(res) == SQLITE_ROW) {
			unread_count = sqlite3_column_int64(res, 0);
		}
		sqlite3_finalize(res);
	}

	return unread_count;
}

static bool
change_unread_status_of_all_items_in_feeds(const struct feed_line **feeds, size_t feeds_count, bool unread)
{
	if (feeds_count == 0) {
		return true;
	}
	struct string *query = crtas("UPDATE items SET unread = ? WHERE feed_url IN (?", 48);
	if (query == NULL) {
		return false;
	}
	for (size_t i = 1; i < feeds_count; ++i) {
		if (catas(query, ",?", 2) == false) {
			free_string(query);
			return false;
		}
	}
	if (catas(query, ");", 2) == false) {
		free_string(query);
		return false;
	}
	sqlite3_stmt *res;
	if (db_prepare(query->ptr, query->len + 1, &res) == false) {
		free_string(query);
		return false;
	}
	sqlite3_bind_int(res, 1, unread);
	for (size_t i = 0; i < feeds_count; ++i) {
		sqlite3_bind_text(res, i + 2, feeds[i]->link->ptr, feeds[i]->link->len, NULL);
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
db_mark_all_items_in_feeds_as_read(const struct feed_line **feeds, size_t feeds_count)
{
	return change_unread_status_of_all_items_in_feeds(feeds, feeds_count, false);
}

bool
db_mark_all_items_in_feeds_as_unread(const struct feed_line **feeds, size_t feeds_count)
{
	return change_unread_status_of_all_items_in_feeds(feeds, feeds_count, true);
}
