#include <stdlib.h>
#include "feedeater.h"

sqlite3 *db;

int
db_init(void)
{
	const char *path = get_db_path();
	if (path == NULL) {
		// Error message is written by get_db_path().
		return 1;
	}
	if (sqlite3_open(path, &db) != SQLITE_OK) {
		fprintf(stderr, "failed to open database!\n");
		sqlite3_close(db);
		return 1; // failure
	}
	sqlite3_exec(
		db,
		"CREATE TABLE IF NOT EXISTS feeds("
			"feed_url TEXT NOT NULL UNIQUE," // url of feed itself
			"title TEXT NOT NULL," // name of feed
			"link TEXT NOT NULL," // url to related resource
			"authors TEXT NOT NULL,"
			"categories TEXT NOT NULL,"
			"summary TEXT NOT NULL,"
			"summary_type TEXT NOT NULL"
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
			"comments TEXT NOT NULL,"
			"summary TEXT NOT NULL,"
			"summary_type TEXT NOT NULL,"
			"content TEXT NOT NULL,"
			"content_type TEXT NOT NULL"
		");"
		"VACUUM;",
		0,
		0,
		NULL
	);
	return 0; // success
}

int
db_begin_transaction(void)
{
	char *errmsg;
	sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, &errmsg);
	if (errmsg != NULL) {
		FAIL("Can not begin transaction: %s", errmsg);
		sqlite3_free(errmsg);
		return 1;
	}
	return 0;
}

int
db_commit_transaction(void)
{
	char *errmsg;
	sqlite3_exec(db, "COMMIT;", NULL, NULL, &errmsg);
	if (errmsg != NULL) {
		FAIL("Can not commit transaction: %s", errmsg);
		sqlite3_free(errmsg);
		return 1;
	}
	return 0;
}

int
db_rollback_transaction(void)
{
	char *errmsg;
	sqlite3_exec(db, "ROLLBACK;", NULL, NULL, &errmsg);
	if (errmsg != NULL) {
		FAIL("Can not rollback transaction: %s", errmsg);
		sqlite3_free(errmsg);
		return 1;
	}
	return 0;
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
