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
db_insert_item(struct item_bucket *bucket, char *feed_url)
{
	sqlite3_stmt *s;
	if (sqlite3_prepare_v2(db, "INSERT INTO items VALUES(?, ?, ?, ?, ?, ?, ?, ?);", -1, &s, 0) == SQLITE_OK) {
		sqlite3_bind_text(s,  ITEM_COLUMN_FEED + 1,    feed_url, strlen(feed_url), NULL);
		sqlite3_bind_text(s,  ITEM_COLUMN_TITLE + 1,   bucket->title.ptr, bucket->title.len - 1, NULL);
		sqlite3_bind_text(s,  ITEM_COLUMN_GUID + 1,    bucket->uid.ptr, bucket->uid.len - 1, NULL);
		sqlite3_bind_int(s,   ITEM_COLUMN_UNREAD + 1,  1);
		sqlite3_bind_text(s,  ITEM_COLUMN_LINK + 1,    bucket->link.ptr, bucket->link.len - 1, NULL);
		sqlite3_bind_text(s,  ITEM_COLUMN_AUTHOR + 1,  bucket->author.ptr, bucket->author.len - 1, NULL);
		sqlite3_bind_int64(s, ITEM_COLUMN_PUBDATE + 1, (sqlite3_int64)(bucket->pubdate));
		sqlite3_bind_text(s,  ITEM_COLUMN_CONTENT + 1, bucket->content.ptr, bucket->content.len - 1, NULL);
	} else {
		fprintf(stderr, "failed to execute INSERT statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_step(s);
	sqlite3_finalize(s);
}

void
db_mark_item_unread(char *feed_url, struct item_entry *item, bool state)
{
	if (item == NULL) return;
	sqlite3_stmt *res;
	char cmd[] = "UPDATE items SET unread = ? WHERE feed = ? AND guid = ? AND link = ?";
	int rc = sqlite3_prepare_v2(db, cmd, -1, &res, 0);
	if (rc == SQLITE_OK) {
		sqlite3_bind_int(res, 1, state == true ? 1 : 0);
		sqlite3_bind_text(res, 2, feed_url, strlen(feed_url), NULL);
		sqlite3_bind_text(res, 3, item->guid, strlen(item->guid), NULL);
		sqlite3_bind_text(res, 4, item->url, strlen(item->url), NULL);
		sqlite3_step(res);
	} else {
		fprintf(stderr, "failed to execute statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(res);
}


void
db_stop(void)
{
	sqlite3_close(db);
}
