#include <stdlib.h>
#include "feedeater.h"

sqlite3 *db;

int
db_init(void)
{
	char *path = get_db_path();
	if (path == NULL) {
		fprintf(stderr, "failed to get to database!\n");
		return 1; // failure
	}
	if (sqlite3_open(path, &db) != SQLITE_OK) {
		fprintf(stderr, "failed to open database!\n");
		sqlite3_close(db);
		free(path);
		return 1; // failure
	}
	sqlite3_exec(
		db,
		"CREATE TABLE IF NOT EXISTS feeds("
			"url TEXT NOT NULL,"
			"name TEXT NOT NULL,"
			"resource TEXT NOT NULL,"
			"description TEXT NOT NULL"
		");"
		"CREATE TABLE IF NOT EXISTS items("
			"feed TEXT NOT NULL,"
			"title TEXT NOT NULL,"
			"guid TEXT NOT NULL,"
			"unread INTEGER(1),"
			"url TEXT NOT NULL,"
			"enclosures TEXT NOT NULL,"
			"authors TEXT NOT NULL,"
			"categories TEXT NOT NULL,"
			"pubdate INTEGER(8),"
			"upddate INTEGER(8),"
			"comments TEXT NOT NULL,"
			"summary TEXT NOT NULL,"
			"summary_type TEXT NOT NULL,"
			"content TEXT NOT NULL,"
			"content_type TEXT NOT NULL"
		");",
		0,
		0,
		NULL
	);
	free(path);
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
