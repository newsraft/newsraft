#include "feedeater.h"

sqlite3 *db;

int
db_init(void)
{
	char *path = get_db_path();
	if (path == NULL) {
		fprintf(stderr, "failed to get to database!\n");
		return 1;
	}
	if (sqlite3_open(path, &db) != SQLITE_OK) {
		fprintf(stderr, "failed to open database!\n");
		sqlite3_close(db);
		free(path);
		return 1;
	}
	char cmd[] = "CREATE TABLE IF NOT EXISTS feeds("
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
	             	"marked INTEGER(1),"
	             	"url TEXT,"
	             	"author TEXT,"
	             	"category TEXT,"
	             	"pubdate INTEGER(8),"
	             	"upddate INTEGER(8),"
	             	"comments TEXT,"
	             	"content TEXT"
	             ");";
	sqlite3_exec(db, cmd, 0, 0, NULL);
	free(path);
	return 0;
}

void
db_stop(void)
{
	sqlite3_close(db);
}

int
db_bind_string(sqlite3_stmt *s, int pos, struct string *str)
{
	if (str != NULL) {
		return sqlite3_bind_text(s, pos, str->ptr, str->len, NULL);
	} else {
		return sqlite3_bind_text(s, pos, NULL, 0, NULL);
	}
}
