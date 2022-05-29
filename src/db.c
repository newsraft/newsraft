#include <string.h>
#include "newsraft.h"

static sqlite3 *db;

bool
db_init(void)
{
	const char *path = get_db_path();
	if (path == NULL) {
		// Error message written by get_db_path.
		return false;
	}

	if (sqlite3_open(path, &db) != SQLITE_OK) {
		fprintf(stderr, "Failed to open database!\n");
		sqlite3_close(db);
		return false;
	}

	char *errmsg;

	// Dates are stored as the number of seconds since 1970.
	sqlite3_exec(
		db,
		"CREATE TABLE IF NOT EXISTS feeds("
			"feed_url TEXT NOT NULL UNIQUE," // url of feed itself
			"title TEXT," // name of feed
			"link TEXT," // url to related resource
			"summary TEXT,"
			"authors TEXT,"
			"editors TEXT,"
			"webmasters TEXT,"
			"categories TEXT,"
			"languages TEXT,"
			"generator TEXT,"
			"rights TEXT,"
			"update_date INTEGER(8) NOT NULL DEFAULT 0,"
			"download_date INTEGER(8) NOT NULL DEFAULT 0,"
			"http_header_etag TEXT," // ETag HTTP header
			"http_header_last_modified INTEGER(8) NOT NULL DEFAULT 0," // Last-Modified HTTP header expressed in epoch time
			"http_header_expires INTEGER(8) NOT NULL DEFAULT 0" // Expires HTTP header expressed in epoch time
		");"
		"CREATE TABLE IF NOT EXISTS items("
			"feed_url TEXT NOT NULL," // url of feed this item belongs to
			"title TEXT," // name of item
			"guid TEXT,"
			"link TEXT," // url to related resource
			"attachments TEXT,"
			"authors TEXT,"
			"categories TEXT,"
			"comments_url TEXT,"
			"summary TEXT,"
			"content TEXT,"
			"locations TEXT,"
			"languages TEXT,"
			"thumbnails TEXT,"
			"publication_date INTEGER(8) NOT NULL DEFAULT 0,"
			"update_date INTEGER(8) NOT NULL DEFAULT 0,"
			"unread INTEGER(1) NOT NULL DEFAULT 0" // 0 if item read and 1 if item unread
		");",
		NULL,
		NULL,
		&errmsg
	);
	if (errmsg != NULL) {
		fprintf(stderr, "Failed to initialize database: %s!\n", errmsg);
		sqlite3_close(db);
		sqlite3_free(errmsg);
		return false;
	}

	if (get_cfg_bool(CFG_CLEAN_DATABASE_ON_STARTUP) == true) {
		sqlite3_exec(db, "VACUUM;", NULL, NULL, &errmsg);
		if (errmsg != NULL) {
			fprintf(stderr, "Failed to clean database: %s!\n", errmsg);
			sqlite3_close(db);
			sqlite3_free(errmsg);
			return false;
		}
	}

	if (get_cfg_bool(CFG_ANALYZE_DATABASE_ON_STARTUP) == true) {
		sqlite3_exec(db, "ANALYZE;", NULL, NULL, &errmsg);
		if (errmsg != NULL) {
			fprintf(stderr, "Failed to analyze database: %s!\n", errmsg);
			sqlite3_close(db);
			sqlite3_free(errmsg);
			return false;
		}
	}

	return true;
}

bool
db_begin_transaction(void)
{
	INFO("Starting database transaction.");
	char *errmsg;
	sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, &errmsg);
	if (errmsg != NULL) {
		FAIL("Can not begin transaction: %s", errmsg);
		sqlite3_free(errmsg);
		return false;
	}
	return true;
}

bool
db_commit_transaction(void)
{
	INFO("Committing database transaction.");
	char *errmsg;
	sqlite3_exec(db, "COMMIT;", NULL, NULL, &errmsg);
	if (errmsg != NULL) {
		FAIL("Can not commit transaction: %s", errmsg);
		sqlite3_free(errmsg);
		return false;
	}
	return true;
}

bool
db_rollback_transaction(void)
{
	INFO("Rolling back database transaction.");
	char *errmsg;
	sqlite3_exec(db, "ROLLBACK;", NULL, NULL, &errmsg);
	if (errmsg != NULL) {
		FAIL("Can not rollback transaction: %s", errmsg);
		sqlite3_free(errmsg);
		return false;
	}
	return true;
}

void
db_stop(void)
{
	sqlite3_close(db);
}

bool
db_prepare(const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail)
{
	INFO("Preparing \"%s\" statement...", zSql);
	if (sqlite3_prepare_v2(db, zSql, nByte, ppStmt, pzTail) != SQLITE_OK) {
		FAIL("Failed to prepare \"%s\" statement: %s", zSql, sqlite3_errmsg(db));
		return false;
	}
	return true;
}

const char *
db_error_string(void)
{
	return sqlite3_errmsg(db);
}

int
db_bind_string(sqlite3_stmt *stmt, int pos, const struct string *str)
{
	if ((str == NULL) || (str->len == 0)) {
		return sqlite3_bind_null(stmt, pos);
	} else {
		return sqlite3_bind_text(stmt, pos, str->ptr, str->len, NULL);
	}
}

struct string *
db_get_plain_text_from_column(sqlite3_stmt *res, int column)
{
	struct string *str = crtes();
	if (str == NULL) {
		return NULL;
	}

	const char *full_text = (const char *)sqlite3_column_text(res, column);
	if (full_text == NULL) {
		return str;
	}

	const size_t full_text_len = strlen(full_text);
	if (full_text_len == 0) {
		return str;
	}

	const char *type_separator = strchr(full_text, ';');
	if (type_separator == NULL) {
		FAIL("Text column value does not have separator character!");
		goto error;
	}
	const size_t type_len = type_separator - full_text;

	const char *real_text = type_separator + 1;
	const size_t real_text_len = full_text_len - (type_len + 1);

	if (cpyas(str, real_text, real_text_len) == false) {
		FAIL("Not enough memory for getting plain text out of column text value!");
		goto error;
	}

	return str;
error:
	free_string(str);
	return NULL;
}

int64_t
db_get_date_from_feeds_table(const struct string *url, const char *column, size_t column_len)
{
	struct string *query = crtas("SELECT ", 7);
	if (query == NULL) {
		FAIL("Not enough memory for query string to get date from feeds table!");
		return -1;
	}
	catas(query, column, column_len);
	catas(query, " FROM feeds WHERE feed_url=?;", 29);
	sqlite3_stmt *res;
	if (db_prepare(query->ptr, query->len + 1, &res, NULL) == false) {
		free_string(query);
		return -1;
	}
	sqlite3_bind_text(res, 1, url->ptr, url->len, NULL);
	int64_t date;
	if (sqlite3_step(res) == SQLITE_ROW) {
		date = sqlite3_column_int64(res, 0);
	} else {
		// This is probably the first reloading of the feed.
		date = 0;
	}
	sqlite3_finalize(res);
	free_string(query);
	return date;
}

struct string *
db_get_string_from_feed_table(const struct string *url, const char *column, size_t column_len)
{
	struct string *str = crtes();
	if (str == NULL) {
		FAIL("Not enough memory for string from feed table!");
		return NULL;
	}
	struct string *query = crtas("SELECT ", 7);
	if (query == NULL) {
		free_string(str);
		return NULL;
	}
	catas(query, column, column_len);
	catas(query, " FROM feeds WHERE feed_url=?;", 29);
	sqlite3_stmt *res;
	if (db_prepare(query->ptr, query->len + 1, &res, NULL) == true) {
		sqlite3_bind_text(res, 1, url->ptr, url->len, NULL);
		if (sqlite3_step(res) == SQLITE_ROW) {
			const char *str_value = (char *)sqlite3_column_text(res, 0);
			if (str_value != NULL) {
				cpyas(str, str_value, strlen(str_value));
			}
		}
		sqlite3_finalize(res);
	}
	free_string(query);
	return str;
}
