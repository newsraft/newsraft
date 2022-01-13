#include "feedeater.h"

static sqlite3 *db;

bool
db_init(void)
{
	const char *path = get_db_path();
	if (path == NULL) {
		// Error message is written by get_db_path().
		return false;
	}
	if (sqlite3_open(path, &db) != SQLITE_OK) {
		fprintf(stderr, "Failed to open database!\n");
		sqlite3_close(db);
		return false;
	}
	char *errmsg;
	sqlite3_exec(
		db,
		"CREATE TABLE IF NOT EXISTS feeds("
			"feed_url TEXT NOT NULL UNIQUE," // url of feed itself
			"title TEXT NOT NULL," // name of feed
			"link TEXT NOT NULL," // url to related resource
			"summary TEXT NOT NULL,"
			"authors TEXT NOT NULL,"
			"editors TEXT NOT NULL,"
			"webmasters TEXT NOT NULL,"
			"categories TEXT NOT NULL,"
			"language TEXT NOT NULL,"
			"generator TEXT NOT NULL,"
			"rights TEXT NOT NULL,"
			"update_time INTEGER(8),"
			"download_time INTEGER(8)"
		");"
		"CREATE TABLE IF NOT EXISTS items("
			"feed_url TEXT NOT NULL," // url of feed this item belongs to
			"title TEXT NOT NULL," // name of item
			"guid TEXT NOT NULL,"
			"link TEXT NOT NULL," // url to related resource
			"unread INTEGER(1)," // 0 if item read and 1 if item unread
			"enclosures TEXT NOT NULL,"
			"authors TEXT NOT NULL,"
			"categories TEXT NOT NULL,"
			"pubdate INTEGER(8)," // publication date in seconds since 1970
			"upddate INTEGER(8)," // update date in seconds since 1970
			"comments_url TEXT NOT NULL,"
			"summary TEXT NOT NULL,"
			"content TEXT NOT NULL"
		");"
		"VACUUM;"
		"ANALYZE;",
		0,
		0,
		&errmsg
	);
	if (errmsg != NULL) {
		fprintf(stderr, "Failed to start database: %s!\n", errmsg);
		sqlite3_free(errmsg);
		return false;
	}
	return true;
}

bool
db_begin_transaction(void)
{
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
