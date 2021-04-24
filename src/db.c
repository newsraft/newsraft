#include <string.h>
#include "feedeater.h"

sqlite3 *db = NULL;

int db_init(void)
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
		return 2;
	}
	char cmd[] = "CREATE TABLE IF NOT EXISTS feeds("
	             	"name TEXT,"
	             	"link TEXT"
	             ");"
	             "CREATE TABLE IF NOT EXISTS items("
	             	"feed TEXT,"
	             	"name TEXT,"
	             	"guid TEXT,"
	             	"unread INTEGER(1),"
	             	"link TEXT,"
	             	"author TEXT,"
	             	"pubdate INTEGER(8),"
	             	"content TEXT"
	             ");";
	sqlite3_exec(db, cmd, 0, 0, NULL);
	free(path);
	return 0;
}

int
db_insert_item(struct item_bucket *bucket, char *feed_url)
{
	int rc, error = 0;
	char cmd[] = "INSERT INTO items VALUES(?, ?, ?, ?, ?, ?, ?, ?);";
	sqlite3_stmt *res;
	rc = sqlite3_prepare_v2(db, cmd, -1, &res, 0);
	if (rc == SQLITE_OK) {
		sqlite3_bind_text(res, 1, feed_url, strlen(feed_url), NULL);
		sqlite3_bind_text(res, 2, bucket->title.ptr, bucket->title.len - 1, NULL);
		sqlite3_bind_text(res, 3, bucket->uid.ptr, bucket->uid.len - 1, NULL);
		sqlite3_bind_int(res, 4, 1);
		sqlite3_bind_text(res, 5, bucket->link.ptr, bucket->link.len - 1, NULL);
		sqlite3_bind_text(res, 6, bucket->author.ptr, bucket->author.len - 1, NULL);
		sqlite3_bind_int64(res, 7, (sqlite3_int64)(bucket->pubdate));
		sqlite3_bind_text(res, 8, bucket->content.ptr, bucket->content.len - 1, NULL);
	} else {
		fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_step(res);
	sqlite3_finalize(res);
	return error;
}

void db_stop(void)
{
	sqlite3_close(db);
}
