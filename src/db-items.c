#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

// On success returns pointer to sqlite3_stmt.
// On failure returns NULL.
sqlite3_stmt *
db_find_item_by_rowid(int rowid)
{
	INFO("Looking for item with rowid %d...", rowid);
	sqlite3_stmt *res;
	if (db_prepare("SELECT * FROM items WHERE rowid = ? LIMIT 1", 44, &res, NULL) == false) {
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
	if (db_prepare(cmd, cmd_size, &res, NULL) == true) {
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

#define SELECT_CMD_START "SELECT COUNT(*) FROM items WHERE unread=1 AND "
#define SELECT_CMD_START_LEN 46

int
get_unread_items_count(const struct set_condition *sc)
{
	INFO("Trying to count unread items by their condition.");

	size_t cmd_size = sizeof(char) * (SELECT_CMD_START_LEN + sc->db_cmd->len + 1);
	char *cmd = malloc(cmd_size);
	if (cmd == NULL) {
		FAIL("Not enough memory for a SQL statement to count unread items!");
		return 0; // failure
	}

	strcpy(cmd, SELECT_CMD_START);
	strcat(cmd, sc->db_cmd->ptr);

	int unread_count = 0;
	sqlite3_stmt *res;
	if (db_prepare(cmd, cmd_size, &res, NULL) == true) {
		for (size_t i = 0; i < sc->urls_count; ++i) {
			sqlite3_bind_text(res, i + 1, sc->urls[i]->ptr, sc->urls[i]->len, NULL);
		}
		if (sqlite3_step(res) == SQLITE_ROW) {
			unread_count = sqlite3_column_int(res, 0);
			INFO("Successfully counted the number of unread items: %d", unread_count);
		}
		sqlite3_finalize(res);
	}

	free(cmd);

	return unread_count;
}
