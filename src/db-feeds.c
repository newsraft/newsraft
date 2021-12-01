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
		DEBUG_WRITE_DB_PREPARE_FAIL;
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
		DEBUG_WRITE_DB_PREPARE_FAIL;
	}
	return created;
}

void
db_update_feed_text(const struct string *feed_url, const char *column, const char *data, size_t data_len)
{
	if (make_sure_feed_is_in_db(feed_url) == false) {
		FAIL("Could not create feed entry in database!");
		return;
	}
	char *cmd = malloc(sizeof(char) * (37 + strlen(column)));
	if (cmd == NULL) {
		return;
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
		DEBUG_WRITE_DB_PREPARE_FAIL;
	}
	free(cmd);
}

#define SELECT_CMD_START "SELECT rowid FROM items WHERE unread=? AND "
#define SELECT_CMD_START_LEN 43

size_t
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

	size_t unread_count = 0;
	sqlite3_stmt *res;
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_int(res, 1, 1);
		for (size_t i = 0; i < sc->urls_count; ++i) {
			sqlite3_bind_text(res, i + 2, sc->urls[i]->ptr, sc->urls[i]->len, NULL);
		}
		while (sqlite3_step(res) == SQLITE_ROW) {
			++unread_count;
		}
		sqlite3_finalize(res);
		INFO("Successfully counted the number of unread items: %zu", unread_count);
	} else {
		DEBUG_WRITE_DB_PREPARE_FAIL;
	}

	free(cmd);
	return unread_count;
}
