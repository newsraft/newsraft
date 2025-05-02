#include <string.h>
#include "insert_feed/insert_feed.h"

bool
delete_excess_items(struct feed_entry *feed, int64_t limit)
{
	char query[300] = "DELETE FROM items WHERE rowid IN "
		"(SELECT rowid FROM items WHERE feed_url=? ORDER BY publication_date DESC, update_date DESC, rowid DESC LIMIT -1 OFFSET ?)";

	if (get_cfg_bool(&feed->cfg, CFG_ITEM_LIMIT_UNREAD) == false) {
		strcat(query, " AND unread=0");
	}
	if (get_cfg_bool(&feed->cfg, CFG_ITEM_LIMIT_IMPORTANT) == false) {
		strcat(query, " AND important=0");
	}

	INFO("Deleting excess items...");
	sqlite3_stmt *s = db_prepare(query, strlen(query) + 1, NULL);
	if (s == NULL) {
		FAIL("Failed to prepare an excess items deletion statement!");
		return false;
	}
	db_bind_string(s, 1, feed->link);
	sqlite3_bind_int64(s, 2, limit);
	sqlite3_step(s);
	sqlite3_finalize(s);
	return true;
}

static inline bool
db_insert_item(struct feed_entry *feed, struct getfeed_item *item, int64_t rowid)
{
	sqlite3_stmt *s;

	if (rowid == -1) {
		s = db_prepare("INSERT INTO items(feed_url,guid,title,link,content,attachments,persons,extras,publication_date,update_date,unread) VALUES(?,?,?,?,?,?,?,?,?,?,1)", 145, NULL);
	} else {
		if (get_cfg_bool(&feed->cfg, CFG_MARK_ITEM_UNREAD_ON_CHANGE) == true) {
			s = db_prepare("UPDATE items SET feed_url=?,guid=?,title=?,link=?,content=?,attachments=?,persons=?,extras=?,publication_date=?,update_date=?,unread=1 WHERE rowid=?", 149, NULL);
		} else {
			s = db_prepare("UPDATE items SET feed_url=?,guid=?,title=?,link=?,content=?,attachments=?,persons=?,extras=?,publication_date=?,update_date=? WHERE rowid=?", 140, NULL);
		}
	}
	if (s == NULL) {
		FAIL("Failed to prepare item insertion/update statement!");
		return false;
	}

	db_bind_string(s,     1 + ITEM_COLUMN_FEED_URL,         feed->link);
	db_bind_string(s,     1 + ITEM_COLUMN_GUID,             item->guid);
	db_bind_string(s,     1 + ITEM_COLUMN_TITLE,            item->title);
	db_bind_string(s,     1 + ITEM_COLUMN_LINK,             STRING_IS_EMPTY(item->url) && item->guid_is_url == true ? item->guid : item->url);
	db_bind_string(s,     1 + ITEM_COLUMN_CONTENT,          item->content);
	db_bind_string(s,     1 + ITEM_COLUMN_ATTACHMENTS,      item->attachments);
	db_bind_string(s,     1 + ITEM_COLUMN_PERSONS,          item->persons);
	db_bind_string(s,     1 + ITEM_COLUMN_EXTRAS,           item->extras);
	sqlite3_bind_int64(s, 1 + ITEM_COLUMN_PUBLICATION_DATE, item->publication_date);
	sqlite3_bind_int64(s, 1 + ITEM_COLUMN_UPDATE_DATE,      item->update_date);
	if (rowid != -1) {
		sqlite3_bind_int64(s, 2 + ITEM_COLUMN_UPDATE_DATE, rowid);
	}

	if (sqlite3_step(s) != SQLITE_DONE) {
		if (rowid == -1) {
			FAIL("Item insertion failed: %s", db_error_string());
		} else {
			FAIL("Item update failed: %s", db_error_string());
		}
		sqlite3_finalize(s);
		return false;
	}

	sqlite3_finalize(s);

	return true;
}

bool
insert_item_data(struct feed_entry *feed, struct getfeed_item *item)
{
	// Create guid if it was not set.
	if (STRING_IS_EMPTY(item->guid)) {
		if (!STRING_IS_EMPTY(item->url)) {
			cpyss(&item->guid, item->url);
		} else if (!STRING_IS_EMPTY(item->title)) {
			cpyss(&item->guid, item->title);
		} else if (!STRING_IS_EMPTY(item->content)) {
			newsraft_simple_hash(&item->guid, item->content->ptr);
		} else {
			WARN("Couldn't generate GUID for the item!");
			return true; // Probably this item is just empty. Ignore it.
		}
	}

	// Before trying to write some item to the database we have to check if this
	// item is duplicate or not.
	sqlite3_stmt *s = db_prepare("SELECT rowid,content FROM items WHERE feed_url=? AND guid=? LIMIT 1", 68, NULL);
	if (s == NULL) {
		FAIL("Failed to prepare SELECT statement for searching item duplicate by guid!");
		return false;
	}
	db_bind_string(s, 1, feed->link);
	db_bind_string(s, 2, item->guid);

	int64_t item_rowid;
	if (sqlite3_step(s) == SQLITE_ROW) {
		const char *content = (char *)sqlite3_column_text(s, 1);
		if (((content == NULL) && (item->content == NULL))
			|| ((content != NULL) && (item->content != NULL)
				&& (strcmp(content, item->content->ptr) == 0)))
		{
			sqlite3_finalize(s);
			return true;
		}
		item_rowid = sqlite3_column_int64(s, 0);
	} else {
		item_rowid = -1;
	}

	sqlite3_finalize(s);

	return db_insert_item(feed, item, item_rowid);
}
