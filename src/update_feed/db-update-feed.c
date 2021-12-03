#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

static bool
is_feed_in_db(const struct string *feed_url)
{
	bool stored = false;
	sqlite3_stmt *s;
	if (sqlite3_prepare_v2(db, "SELECT * FROM feeds WHERE url = ? LIMIT 1", -1, &s, 0) == SQLITE_OK) {
		sqlite3_bind_text(s, 1, feed_url->ptr, feed_url->len, NULL);
		if (sqlite3_step(s) == SQLITE_ROW) {
			stored = true;
		}
		sqlite3_finalize(s);
	} else {
		FAIL_SQLITE_PREPARE;
	}
	return stored;
}

static bool
make_sure_feed_is_in_db(const struct string *feed_url)
{
	if (is_feed_in_db(feed_url) == true) {
		return true;
	}
	bool created = false;
	sqlite3_stmt *s;
	if (sqlite3_prepare_v2(db, "INSERT INTO feeds (url) VALUES(?)", -1, &s, 0) == SQLITE_OK) {
		sqlite3_bind_text(s, 1, feed_url->ptr, feed_url->len, NULL);
		if (sqlite3_step(s) == SQLITE_DONE) {
			created = true;
		}
		sqlite3_finalize(s);
	} else {
		FAIL_SQLITE_PREPARE;
	}
	return created;
}

void
db_update_feed_text(const struct string *feed_url, const char *column, const char *data, size_t data_len)
{
	if (make_sure_feed_is_in_db(feed_url) == false) {
		FAIL("Can not create feed entry in database!");
		return; // failure
	}

	char *cmd = malloc(sizeof(char) * (37 + strlen(column)));
	if (cmd == NULL) {
		return; // failure
	}

	// BEFORE CHANGING FORMAT STRING LO^OK ABOVE
	sprintf(cmd, "UPDATE feeds SET %s = ? WHERE url = ?", column);
	sqlite3_stmt *res;
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_text(res, 1, data, data_len, NULL);
		sqlite3_bind_text(res, 2, feed_url->ptr, feed_url->len, NULL);
		sqlite3_step(res);
		sqlite3_finalize(res);
	} else {
		FAIL_SQLITE_PREPARE;
	}

	free(cmd);
}
