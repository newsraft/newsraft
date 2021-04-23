#include <string.h>
#include "feedeater.h"
#include "db/db.h"

sqlite3 *db = NULL;

void
mark_read(char *item_path)
{
	char *item = malloc(sizeof(char) * MAXPATH);
	if (item == NULL) return;
	strcpy(item, item_path);
	strcat(item, ISNEW_FILE);
	remove(item);
	free(item);
}

void
mark_unread(char *item_path)
{
	char *item = malloc(sizeof(char) * MAXPATH);
	if (item == NULL) return;
	strcpy(item, item_path);
	strcat(item, ISNEW_FILE);
	// whooooooooooaaaa, that's how file is created. lol
	fclose(fopen(item, "w"));
	free(item);
}

int
try_bucket(struct item_bucket *bucket, char *feed_url)
{
	if (bucket == NULL) return 0;
	int is_unique = 0, rc;
	sqlite3_stmt *res;
	if (bucket->uid.ptr != NULL && bucket->uid.len != 0) {
		char cmd1[] = "SELECT * FROM items WHERE guid = ?";
		rc = sqlite3_prepare_v2(db, cmd1, -1, &res, 0);
		if (rc == SQLITE_OK) {
			sqlite3_bind_text(res, 1, bucket->uid.ptr, bucket->uid.len - 1, NULL);
			if (sqlite3_step(res) == SQLITE_DONE) is_unique = 1;
		} else {
			fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
		}
		sqlite3_finalize(res);
	} else if (bucket->link.ptr != NULL && bucket->link.len != 0) {
		char cmd2[] = "SELECT * FROM items WHERE link = ?";
		rc = sqlite3_prepare_v2(db, cmd2, -1, &res, 0);
		if (rc == SQLITE_OK) {
			sqlite3_bind_text(res, 1, bucket->link.ptr, bucket->link.len - 1, NULL);
			if (sqlite3_step(res) == SQLITE_DONE) is_unique = 1;
		} else {
			fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
		}
		sqlite3_finalize(res);
	}
	if (is_unique) {
		char cmd3[] = "INSERT INTO items VALUES(?, ?, ?, ?, ?, ?, ?);";
		rc = sqlite3_prepare_v2(db, cmd3, -1, &res, 0);
		if (rc == SQLITE_OK) {
			sqlite3_bind_text(res, 1, feed_url, strlen(feed_url), NULL);
			sqlite3_bind_text(res, 2, bucket->title.ptr, bucket->title.len - 1, NULL);
			sqlite3_bind_text(res, 3, bucket->uid.ptr, bucket->uid.len - 1, NULL);
			sqlite3_bind_int(res, 4, 1);
			sqlite3_bind_text(res, 5, bucket->link.ptr, bucket->link.len - 1, NULL);
			sqlite3_bind_text(res, 6, bucket->author.ptr, bucket->author.len - 1, NULL);
			sqlite3_bind_text(res, 7, bucket->content.ptr, bucket->content.len - 1, NULL);
		} else {
			fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
		}
		sqlite3_step(res);
		sqlite3_finalize(res);
	}
	return 1;
}

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
	             	"content TEXT"
	             ");";
	sqlite3_exec(db, cmd, 0, 0, NULL);
	free(path);
	return 0;
}

void db_stop(void)
{
	sqlite3_close(db);
}
