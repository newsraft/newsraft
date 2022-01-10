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

int
db_prepare(const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail)
{
	INFO("Preparing \"%s\" statement...", zSql);
	int result = sqlite3_prepare_v2(db, zSql, nByte, ppStmt, pzTail);
	if (result != SQLITE_OK) {
		FAIL("Failed to prepare \"%s\" statement: %s", zSql, sqlite3_errmsg(db));
	}
	return result;
}

const char *
db_error_string(void)
{
	return sqlite3_errmsg(db);
}
