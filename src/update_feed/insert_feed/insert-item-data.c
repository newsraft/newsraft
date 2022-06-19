#include <string.h>
#include "update_feed/insert_feed/insert_feed.h"

static struct string *
fnv_1a_string(const char *src)
{
	// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
	// http://isthe.com/chongo/tech/comp/fnv
	// https://datatracker.ietf.org/doc/html/draft-eastlake-fnv-17.html
	uint64_t hash = 14695981039346656037LLU;
	for (const char *i = src; *i != '\0'; ++i) {
		hash = (hash ^ *i) * 1099511628211LLU;
	}
	char str[60];
	size_t str_len = 47 + hash % 13, k = 0;
	while (k < str_len) {
		str[k] = hash % 94 + 33;
		hash = (hash ^ str[k++]) * 109951183333LLU;
	}
	str[str_len] = '\0';
	return crtas(str, str_len);
}

bool
delete_excess_items(const struct string *feed_url, int64_t limit)
{
	INFO("Deleting excess items...");
	sqlite3_stmt *s;
	if (db_prepare("DELETE FROM items WHERE rowid IN (SELECT rowid FROM items WHERE feed_url = ? ORDER BY publication_date DESC, update_date DESC, rowid DESC LIMIT -1 OFFSET ?);", 158, &s) == false) {
		FAIL("Failed to prepare an excess items deletion statement!");
		return false;
	}
	db_bind_string(s, 1, feed_url);
	sqlite3_bind_int64(s, 2, limit);
	sqlite3_step(s);
	sqlite3_finalize(s);
	return true;
}

static inline bool
db_insert_item(const struct string *feed_url, struct getfeed_item *item, int64_t rowid)
{
	sqlite3_stmt *s;

	if (rowid == -1) {
		if (db_prepare("INSERT INTO items VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", 69, &s) == false) {
			FAIL("Failed to prepare item insertion statement!");
			return false;
		}
	} else {
		if (db_prepare("UPDATE items SET feed_url = ?, guid = ?, title = ?, link = ?, summary = ?, content = ?, attachments = ?, persons = ?, categories = ?, locations = ?, pictures = ?, publication_date = ?, update_date = ?, unread = ? WHERE rowid = ?;", 230, &s) == false)
		{
			FAIL("Failed to prepare item update statement!");
			return false;
		}
	}

	db_bind_string(s,      1 + ITEM_COLUMN_FEED_URL,         feed_url);
	db_bind_string(s,      1 + ITEM_COLUMN_GUID,             item->guid);
	db_bind_text_struct(s, 1 + ITEM_COLUMN_TITLE,            &item->title);
	db_bind_string(s,      1 + ITEM_COLUMN_LINK,             item->url);
	db_bind_text_struct(s, 1 + ITEM_COLUMN_SUMMARY,          &item->summary);
	db_bind_text_struct(s, 1 + ITEM_COLUMN_CONTENT,          &item->content);
	db_bind_string(s,      1 + ITEM_COLUMN_ATTACHMENTS,      item->attachments);
	db_bind_string(s,      1 + ITEM_COLUMN_PERSONS,          item->persons);
	db_bind_string(s,      1 + ITEM_COLUMN_CATEGORIES,       item->categories);
	db_bind_string(s,      1 + ITEM_COLUMN_LOCATIONS,        item->locations);
	db_bind_string(s,      1 + ITEM_COLUMN_PICTURES,         item->pictures);
	sqlite3_bind_int64(s,  1 + ITEM_COLUMN_PUBLICATION_DATE, (sqlite3_int64)(item->pubdate));
	sqlite3_bind_int64(s,  1 + ITEM_COLUMN_UPDATE_DATE,      (sqlite3_int64)(item->upddate));
	sqlite3_bind_int(s,    1 + ITEM_COLUMN_UNREAD,           1);
	if (rowid != -1) {
		sqlite3_bind_int64(s, 1 + ITEM_COLUMN_NONE, rowid);
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
insert_item_data(const struct string *feed_url, struct getfeed_item *item)
{
	// Create guid if it was not set.
	if ((item->guid == NULL) || (item->guid->len == 0)) {
		struct string *hash;
		if ((item->url != NULL) && (item->url->len != 0)) {
			if (crtss_or_cpyss(&item->guid, item->url) == false) {
				return false;
			}
		} else if ((item->title.value != NULL) && (item->title.value->len != 0)) {
			if (crtss_or_cpyss(&item->guid, item->title.value) == false) {
				return false;
			}
		} else if ((item->summary.value != NULL) && (item->summary.value->len != 0)) {
			hash = fnv_1a_string(item->summary.value->ptr);
			if (hash == NULL) {
				return false;
			}
			if (crtss_or_cpyss(&item->guid, hash) == false) {
				free_string(hash);
				return false;
			}
			free_string(hash);
		} else if ((item->content.value != NULL) && (item->content.value->len != 0)) {
			hash = fnv_1a_string(item->content.value->ptr);
			if (hash == NULL) {
				return false;
			}
			if (crtss_or_cpyss(&item->guid, hash) == false) {
				free_string(hash);
				return false;
			}
			free_string(hash);
		} else {
			WARN("Couldn't generate GUID for the item!");
			return true; // Probably this item is just empty. Ignore it.
		}
	}

	sqlite3_stmt *s;
	// Before trying to write some item to the database we have to check if this
	// item is duplicate or not.
	if (db_prepare("SELECT rowid, summary, content FROM items WHERE feed_url = ? AND guid = ? LIMIT 1;", 83, &s) == false) {
		FAIL("Failed to prepare SELECT statement for searching item duplicate by guid!");
		return false;
	}
	db_bind_string(s, 1, feed_url);
	db_bind_string(s, 2, item->guid);

	int64_t item_rowid;
	if (sqlite3_step(s) == SQLITE_ROW) {
		const char *typesep;
		const char *summary = (const char *)sqlite3_column_text(s, 1);
		const char *content = (const char *)sqlite3_column_text(s, 2);
		if (summary != NULL) {
			typesep = strchr(summary, ';');
			if (typesep != NULL) {
				summary = typesep + 1;
			}
		}
		if (content != NULL) {
			typesep = strchr(content, ';');
			if (typesep != NULL) {
				content = typesep + 1;
			}
		}
		// Fast explanation: item's summary and content are equal to
		// summary and content of the entry in the database.
		if ((((content == NULL) && (item->content.value == NULL))
				|| ((content != NULL) && (item->content.value != NULL)
					&& (strcmp(content, item->content.value->ptr) == 0)))
			&& (((summary == NULL) && (item->summary.value == NULL))
				|| ((summary != NULL) && (item->summary.value != NULL)
					&& (strcmp(summary, item->summary.value->ptr) == 0))))
		{
			sqlite3_finalize(s);
			return true;
		}
		item_rowid = sqlite3_column_int64(s, 0);
	} else {
		item_rowid = -1;
	}

	sqlite3_finalize(s);

	return db_insert_item(feed_url, item, item_rowid);
}
