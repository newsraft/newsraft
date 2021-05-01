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
	             	"url TEXT,"
	             	"name TEXT,"
	             	"resource TEXT,"
	             	"builddate INTEGER(8),"
	             	"language TEXT,"
	             	"description TEXT"
	             ");"
	             "CREATE TABLE IF NOT EXISTS items("
	             	"feed TEXT,"
	             	"title TEXT,"
	             	"guid TEXT,"
	             	"unread INTEGER(1),"
	             	"url TEXT,"
	             	"author TEXT,"
	             	"category TEXT,"
	             	"pubdate INTEGER(8),"
	             	"comments TEXT,"
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

static void
db_insert_item(struct item_bucket *bucket, struct string *feed_url)
{
	sqlite3_stmt *s;
	if (sqlite3_prepare_v2(db, "INSERT INTO items VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", -1, &s, 0) == SQLITE_OK) {
		sqlite3_bind_text(s,  ITEM_COLUMN_FEED + 1,     feed_url->ptr, feed_url->len, NULL);
		db_bind_string(s,     ITEM_COLUMN_TITLE + 1,    bucket->title);
		db_bind_string(s,     ITEM_COLUMN_GUID + 1,     bucket->guid);
		sqlite3_bind_int(s,   ITEM_COLUMN_UNREAD + 1,   1);
		db_bind_string(s,     ITEM_COLUMN_URL + 1,      bucket->url);
		db_bind_string(s,     ITEM_COLUMN_AUTHOR + 1,   bucket->author);
		db_bind_string(s,     ITEM_COLUMN_CATEGORY + 1, bucket->category);
		sqlite3_bind_int64(s, ITEM_COLUMN_PUBDATE + 1,  (sqlite3_int64)(bucket->pubdate));
		db_bind_string(s,     ITEM_COLUMN_COMMENTS + 1, bucket->comments);
		db_bind_string(s,     ITEM_COLUMN_CONTENT + 1,  bucket->content);
	} else {
		fprintf(stderr, "failed to execute INSERT statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_step(s);
	sqlite3_finalize(s);
}

int
try_item_bucket(struct item_bucket *bucket, struct string *feed_url)
{
	if (bucket == NULL || feed_url == NULL) return 0;
	bool is_item_unique = false;
	sqlite3_stmt *res;
	if (bucket->guid != NULL) { // check uniqueness by guid
		char cmd[] = "SELECT * FROM items WHERE feed = ? AND guid = ? LIMIT 1";
		if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
			sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
			sqlite3_bind_text(res, 2, bucket->guid->ptr, bucket->guid->len, NULL);
			if (sqlite3_step(res) == SQLITE_DONE) is_item_unique = true;
		} else {
			fprintf(stderr, "failed to execute statement: %s\n", sqlite3_errmsg(db));
		}
	} else if (bucket->pubdate > 0) { // check uniqueness by publication date
		char cmd[] = "SELECT * FROM items WHERE feed = ? AND pubdate = ? LIMIT 1";
		if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
			sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
			sqlite3_bind_int64(res, 2, (sqlite3_int64)(bucket->pubdate));
			if (sqlite3_step(res) == SQLITE_DONE) is_item_unique = true;
		} else {
			fprintf(stderr, "failed to execute statement: %s\n", sqlite3_errmsg(db));
		}
	} else if (bucket->url != NULL) { // check uniqueness by url
		char cmd[] = "SELECT * FROM items WHERE feed = ? AND url = ? LIMIT 1";
		if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
			sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
			sqlite3_bind_text(res, 2, bucket->url->ptr, bucket->url->len, NULL);
			if (sqlite3_step(res) == SQLITE_DONE) is_item_unique = true;
		} else {
			fprintf(stderr, "failed to execute statement: %s\n", sqlite3_errmsg(db));
		}
	} else if (bucket->content != NULL) { // check uniqueness by content
		char cmd[] = "SELECT * FROM items WHERE feed = ? AND content = ? LIMIT 1";
		if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
			sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
			sqlite3_bind_text(res, 2, bucket->content->ptr, bucket->content->len, NULL);
			if (sqlite3_step(res) == SQLITE_DONE) is_item_unique = true;
		} else {
			fprintf(stderr, "failed to execute statement: %s\n", sqlite3_errmsg(db));
		}
	}
	if (is_item_unique == true) db_insert_item(bucket, feed_url);
	sqlite3_finalize(res);
	return 1;
}

int
db_mark_item_unread(struct string *feed_url, struct item_entry *item, bool state)
{
	int success = 0;
	if (item == NULL) return success;
	sqlite3_stmt *res;
	char cmd[] = "UPDATE items SET unread = ? WHERE feed = ? AND guid = ? AND url = ?";
	int rc = sqlite3_prepare_v2(db, cmd, -1, &res, 0);
	if (rc == SQLITE_OK) {
		sqlite3_bind_int(res, 1, state == true ? 1 : 0);
		sqlite3_bind_text(res, 2, feed_url->ptr, feed_url->len, NULL);
		sqlite3_bind_text(res, 3, item->guid, strlen(item->guid), NULL);
		sqlite3_bind_text(res, 4, item->url, strlen(item->url), NULL);
		if (sqlite3_step(res) == SQLITE_DONE) success = 1;
	} else {
		fprintf(stderr, "failed to prepare statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(res);
	return success;
}

static bool
is_feed_stored(struct string *feed_url)
{
	bool stored = true;
	sqlite3_stmt *res;
	if (sqlite3_prepare_v2(db, "SELECT * FROM feeds WHERE url = ? LIMIT 1", -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
		if (sqlite3_step(res) == SQLITE_DONE) stored = false;
	} else {
		fprintf(stderr, "failed to execute statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(res);
	return stored;
}

static bool
create_feed_entry(struct string *feed_url)
{
	if (is_feed_stored(feed_url) == true) return true;
	bool created = false;
	sqlite3_stmt *res;
	if (sqlite3_prepare_v2(db, "INSERT INTO feeds (url) VALUES(?)", -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
		if (sqlite3_step(res) == SQLITE_DONE) created = true;
	} else {
		fprintf(stderr, "failed to execute statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(res);
	return created;
}

void
db_update_feed_text(struct string *feed_url, char *column, char *data, size_t data_len)
{
	if (create_feed_entry(feed_url) == false) {
		fprintf(stderr, "could not create feed entry in database!\n"); return;
	}
	char *cmd = malloc(sizeof(char) * (37 + strlen(column)));
	if (cmd == NULL) return;
	sqlite3_stmt *res;
	// BEFORE CHANGING FORMAT STRING LO^OK ABOVE
	sprintf(cmd, "UPDATE feeds SET %s = ? WHERE url = ?", column);
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_text(res, 1, data, data_len, NULL);
		sqlite3_bind_text(res, 2, feed_url->ptr, feed_url->len, NULL);
		/*if (sqlite3_step(res) == SQLITE_DONE) success = 1;*/
		sqlite3_step(res);
	} else {
		fprintf(stderr, "failed to execute statement: %s\n", sqlite3_errmsg(db));
	}
	free(cmd);
	sqlite3_finalize(res);
}

void
db_update_feed_int64(struct string *feed_url, char *column, int64_t i)
{
	if (create_feed_entry(feed_url) == false) {
		fprintf(stderr, "could not create feed entry in database!\n"); return;
	}
	char *cmd = malloc(sizeof(char) * (37 + strlen(column)));
	if (cmd == NULL) return;
	sqlite3_stmt *res;
	// BEFORE CHANGING FORMAT STRING LO^OK ABOVE
	sprintf(cmd, "UPDATE feeds SET %s = ? WHERE url = ?", column);
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		sqlite3_bind_int64(res, 1, i);
		sqlite3_bind_text(res, 2, feed_url->ptr, feed_url->len, NULL);
		/*if (sqlite3_step(res) == SQLITE_DONE) success = 1;*/
		sqlite3_step(res);
	} else {
		fprintf(stderr, "failed to execute statement: %s\n", sqlite3_errmsg(db));
	}
	free(cmd);
	sqlite3_finalize(res);
}

void
db_stop(void)
{
	sqlite3_close(db);
}
