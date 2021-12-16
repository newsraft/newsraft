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
			"url TEXT,"
			"name TEXT,"
			"resource TEXT,"
			"description TEXT"
		");"
		"CREATE TABLE IF NOT EXISTS items("
			"feed TEXT,"
			"title TEXT,"
			"guid TEXT,"
			"unread INTEGER(1),"
			"url TEXT,"
			"enclosures TEXT,"
			"authors TEXT,"
			"categories TEXT,"
			"pubdate INTEGER(8),"
			"upddate INTEGER(8),"
			"comments TEXT,"
			"summary TEXT,"
			"summary_type TEXT,"
			"content TEXT,"
			"content_type TEXT"
		");",
		0,
		0,
		NULL
	);
	free(path);
	return 0; // success
}

void
db_stop(void)
{
	sqlite3_close(db);
}
