#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

static inline bool
is_feed_in_db(const struct string *feed_url)
{
	sqlite3_stmt *s;
	if (db_prepare("SELECT * FROM feeds WHERE url = ? LIMIT 1", 42, &s, NULL) == SQLITE_OK) {
		sqlite3_bind_text(s, 1, feed_url->ptr, feed_url->len, NULL);
		if (sqlite3_step(s) == SQLITE_ROW) {
			sqlite3_finalize(s);
			return true;
		}
		sqlite3_finalize(s);
	}
	return false;
}

static inline bool
make_sure_feed_is_in_db(const struct string *feed_url)
{
	if (is_feed_in_db(feed_url) == true) {
		return true;
	}
	sqlite3_stmt *s;
	if (db_prepare("INSERT INTO feeds VALUES(?, ?, ?, ?)", 37, &s, NULL) == SQLITE_OK) {
		sqlite3_bind_text(s, 1, feed_url->ptr, feed_url->len, NULL);
		sqlite3_bind_text(s, 2, "", 0, NULL);
		sqlite3_bind_text(s, 3, "", 0, NULL);
		sqlite3_bind_text(s, 4, "", 0, NULL);
		if (sqlite3_step(s) == SQLITE_DONE) {
			sqlite3_finalize(s);
			return true;
		}
		FAIL("Failed to insert new feed entry: %s", sqlite3_errmsg(db));
		sqlite3_finalize(s);
	}
	return false;
}

void
db_update_feed_text(const struct string *feed_url, const char *column, const char *value, size_t value_len)
{
	INFO("Updating \"%s\" column of feed \"%s\" with value \"%s\"...", column, feed_url->ptr, value);

	if (make_sure_feed_is_in_db(feed_url) == false) {
		FAIL("Can not create feed entry for \"%s\" in database!", feed_url->ptr);
		return; // failure
	}

	char *cmd = malloc(sizeof(char) * (37 + strlen(column)));
	if (cmd == NULL) {
		return; // failure
	}

	// BEFORE CHANGING FORMAT STRING LO^OK ABOVE
	sprintf(cmd, "UPDATE feeds SET %s = ? WHERE url = ?", column);
	sqlite3_stmt *res;
	if (db_prepare(cmd, -1, &res, NULL) == SQLITE_OK) {
		sqlite3_bind_text(res, 1, value, value_len, NULL);
		sqlite3_bind_text(res, 2, feed_url->ptr, feed_url->len, NULL);
		sqlite3_step(res);
		sqlite3_finalize(res);
	}

	free(cmd);
}
