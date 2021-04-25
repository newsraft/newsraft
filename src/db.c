#include <string.h>
#include "feedeater.h"

sqlite3 *db = NULL;

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

void
db_bind_string(sqlite3_stmt *s, int pos, struct string *str)
{
	if (str != NULL) {
		sqlite3_bind_text(s, pos, str->ptr, str->len, NULL);
	} else {
		sqlite3_bind_text(s, pos, NULL, 0, NULL);
	}
}

int
try_item_bucket(struct item_bucket *bucket, char *feed_url)
{
	if (bucket == NULL || feed_url == NULL) return 0;
	sqlite3_stmt *res;
	char cmd[] = "SELECT * FROM items WHERE feed = ? AND guid = ? AND link = ?";
	int rc = sqlite3_prepare_v2(db, cmd, -1, &res, 0);
	if (rc == SQLITE_OK) {
		sqlite3_bind_text(res, 1, feed_url, strlen(feed_url), NULL);
		db_bind_string(res, 2, bucket->uid);
		db_bind_string(res, 3, bucket->link);
		// if nothing found (item is unique), insert item into table
		if (sqlite3_step(res) == SQLITE_DONE) db_insert_item(bucket, feed_url);
	} else {
		fprintf(stderr, "failed to execute statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(res);
	return 1;
}

void
db_insert_item(struct item_bucket *bucket, char *feed_url)
{
	sqlite3_stmt *s;
	if (sqlite3_prepare_v2(db, "INSERT INTO items VALUES(?, ?, ?, ?, ?, ?, ?, ?);", -1, &s, 0) == SQLITE_OK) {
		sqlite3_bind_text(s,  ITEM_COLUMN_FEED + 1,    feed_url, strlen(feed_url), NULL);
		db_bind_string(s,     ITEM_COLUMN_TITLE + 1,   bucket->title);
		db_bind_string(s,     ITEM_COLUMN_GUID + 1,    bucket->uid);
		sqlite3_bind_int(s,   ITEM_COLUMN_UNREAD + 1,  1);
		db_bind_string(s,     ITEM_COLUMN_LINK + 1,    bucket->link);
		db_bind_string(s,     ITEM_COLUMN_AUTHOR + 1,  bucket->author);
		sqlite3_bind_int64(s, ITEM_COLUMN_PUBDATE + 1, (sqlite3_int64)(bucket->pubdate));
		db_bind_string(s,     ITEM_COLUMN_CONTENT + 1, bucket->content);
	} else {
		fprintf(stderr, "failed to execute INSERT statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_step(s);
	sqlite3_finalize(s);
}

int
db_mark_item_unread(char *feed_url, struct item_entry *item, bool state)
{
	int success = 0;
	if (item == NULL) return success;
	sqlite3_stmt *res;
	char cmd[] = "UPDATE items SET unread = ? WHERE feed = ? AND guid = ? AND link = ?";
	int rc = sqlite3_prepare_v2(db, cmd, -1, &res, 0);
	if (rc == SQLITE_OK) {
		sqlite3_bind_int(res, 1, state == true ? 1 : 0);
		sqlite3_bind_text(res, 2, feed_url, strlen(feed_url), NULL);
		sqlite3_bind_text(res, 3, item->guid, strlen(item->guid), NULL);
		sqlite3_bind_text(res, 4, item->url, strlen(item->url), NULL);
		if (sqlite3_step(res) == SQLITE_DONE) success = 1;
	} else {
		fprintf(stderr, "failed to execute statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(res);
	return success;
}


void
db_stop(void)
{
	sqlite3_close(db);
}
