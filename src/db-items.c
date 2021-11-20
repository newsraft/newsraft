#include <string.h>
#include "feedeater.h"

int
db_update_item_int(int rowid, const char *column, int value)
{
	int success = 0;
	char cmd[100];
	sqlite3_stmt *res;
	strcpy(cmd, "UPDATE items SET ");
	strcat(cmd, column);
	strcat(cmd, " = ? WHERE rowid = ?");
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_int(res, 1, value);
		sqlite3_bind_int(res, 2, rowid);
		if (sqlite3_step(res) == SQLITE_DONE) {
			success = 1;
		}
		sqlite3_finalize(res);
	} else {
		DEBUG_WRITE_DB_PREPARE_FAIL;
	}
	return success;
}
