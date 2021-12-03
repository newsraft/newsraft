#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

static int
db_update_item_int(int rowid, const char *column, int value)
{
	INFO("Updating column \"%s\" with integer value \"%d\" of item with rowid \"%d\".", column, value, rowid);

	// Size of this buffer depends on the size of strings below.
	char *cmd = malloc(sizeof(char) * (17 + strlen(column) + 20 + 1));
	if (cmd == NULL) {
		FAIL("Not enough memory for updating that column!");
		return 1; // failure
	}
	strcpy(cmd, "UPDATE items SET ");
	strcat(cmd, column);
	strcat(cmd, " = ? WHERE rowid = ?");

	int error = 1;

	sqlite3_stmt *res;
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_int(res, 1, value);
		sqlite3_bind_int(res, 2, rowid);
		if (sqlite3_step(res) == SQLITE_DONE) {
			error = 0;
		} else {
			WARN("For some reason column was not updated with new value!");
		}
		sqlite3_finalize(res);
	} else {
		FAIL_SQLITE_PREPARE;
	}

	free(cmd);

	return error;
}

int
db_mark_item_read(int rowid)
{
	return db_update_item_int(rowid, "unread", 0);
}

int
db_mark_item_unread(int rowid)
{
	return db_update_item_int(rowid, "unread", 1);
}

#define SELECT_CMD_START "SELECT COUNT(*) FROM items WHERE unread=? AND "
#define SELECT_CMD_START_LEN 46

int
get_unread_items_count(const struct set_condition *sc)
{
	INFO("Trying to count unread items by their condition.");

	if (sc == NULL) {
		WARN("Condition is unset, can not count unread items!");
		return 0; // failure
	}

	char *cmd = malloc(sizeof(char) * (SELECT_CMD_START_LEN + sc->db_cmd->len + 1));
	if (cmd == NULL) {
		FAIL("Not enough memory for a SQL statement to count unread items!");
		return 0; // failure
	}

	strcpy(cmd, SELECT_CMD_START);
	strcat(cmd, sc->db_cmd->ptr);

	int unread_count = 0;
	sqlite3_stmt *res;
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_int(res, 1, 1);
		for (size_t i = 0; i < sc->urls_count; ++i) {
			sqlite3_bind_text(res, i + 2, sc->urls[i]->ptr, sc->urls[i]->len, NULL);
		}
		if (sqlite3_step(res) == SQLITE_ROW) {
			unread_count = sqlite3_column_int(res, 0);
			INFO("Successfully counted the number of unread items: %d", unread_count);
		}
		sqlite3_finalize(res);
	} else {
		FAIL_SQLITE_PREPARE;
	}

	free(cmd);

	return unread_count;
}
