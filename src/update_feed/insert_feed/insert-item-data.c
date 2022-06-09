#include "update_feed/insert_feed/insert_feed.h"

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
db_insert_item(const struct string *feed_url, struct getfeed_item *item, int rowid)
{
	struct string *authors_str = generate_person_list_string(item->author);
	if (authors_str == NULL) {
		FAIL("Not enough memory for creating authors list of item bucket!");
		return false;
	}

	struct string *attachments_str = generate_link_list_string(item->attachment);
	if (attachments_str == NULL) {
		FAIL("Not enough memory for creating attachments list of item bucket!");
		free_string(authors_str);
		return false;
	}

	struct string *sources_str = generate_source_list_string(item->source);
	if (sources_str == NULL) {
		FAIL("Not enough memory for creating sources list of item bucket!");
		free_string(attachments_str);
		free_string(authors_str);
		return false;
	}

	struct string *categories_str = generate_category_list_string(item->category);
	if (categories_str == NULL) {
		FAIL("Not enough memory for creating categories list of item bucket!");
		free_string(sources_str);
		free_string(attachments_str);
		free_string(authors_str);
		return false;
	}

	struct string *locations_str = concatenate_strings_of_string_list_into_one_string(item->location);
	if (locations_str == NULL) {
		free_string(categories_str);
		free_string(sources_str);
		free_string(attachments_str);
		free_string(authors_str);
		return false;
	}

	struct string *thumbnails_str = generate_picture_list_string(item->thumbnail);
	if (thumbnails_str == NULL) {
		free_string(locations_str);
		free_string(categories_str);
		free_string(sources_str);
		free_string(attachments_str);
		free_string(authors_str);
		return false;
	}

	sqlite3_stmt *s;
	bool prepare_status;

	if (rowid == -1) {
		prepare_status = db_prepare("INSERT INTO items VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", 84, &s);
	} else {
		prepare_status = db_prepare("UPDATE items SET feed_url = ?, guid = ?, title = ?, link = ?, summary = ?, content = ?, attachments = ?, sources = ?, authors = ?, comments_url = ?, locations = ?, categories = ?, languages = ?, rights = ?, rating = ?, thumbnails = ?, publication_date = ?, update_date = ?, unread = ? WHERE rowid = ?;", 302, &s);
	}

	if (prepare_status == false) {
		FAIL("Failed to prepare item insertion statement!");
		free_string(thumbnails_str);
		free_string(locations_str);
		free_string(categories_str);
		free_string(sources_str);
		free_string(attachments_str);
		free_string(authors_str);
		return false;
	}

	db_bind_string(s,      1 + ITEM_COLUMN_FEED_URL,         feed_url);
	db_bind_string(s,      1 + ITEM_COLUMN_GUID,             item->guid);
	db_bind_text_struct(s, 1 + ITEM_COLUMN_TITLE,            &item->title);
	db_bind_string(s,      1 + ITEM_COLUMN_LINK,             item->url);
	db_bind_text_struct(s, 1 + ITEM_COLUMN_SUMMARY,          &item->summary);
	db_bind_text_struct(s, 1 + ITEM_COLUMN_CONTENT,          &item->content);
	db_bind_string(s,      1 + ITEM_COLUMN_ATTACHMENTS,      attachments_str);
	db_bind_string(s,      1 + ITEM_COLUMN_SOURCES,          sources_str);
	db_bind_string(s,      1 + ITEM_COLUMN_AUTHORS,          authors_str);
	db_bind_string(s,      1 + ITEM_COLUMN_COMMENTS_URL,     item->comments_url);
	db_bind_string(s,      1 + ITEM_COLUMN_LOCATIONS,        locations_str);
	db_bind_string(s,      1 + ITEM_COLUMN_CATEGORIES,       categories_str);
	db_bind_string(s,      1 + ITEM_COLUMN_LANGUAGES,        item->language);
	db_bind_text_struct(s, 1 + ITEM_COLUMN_RIGHTS,           &item->rights);
	db_bind_string(s,      1 + ITEM_COLUMN_RATING,           item->rating);
	db_bind_string(s,      1 + ITEM_COLUMN_THUMBNAILS,       thumbnails_str);
	sqlite3_bind_int64(s,  1 + ITEM_COLUMN_PUBLICATION_DATE, (sqlite3_int64)(item->pubdate));
	sqlite3_bind_int64(s,  1 + ITEM_COLUMN_UPDATE_DATE,      (sqlite3_int64)(item->upddate));
	sqlite3_bind_int(s,    1 + ITEM_COLUMN_UNREAD,           1);
	if (rowid != -1) {
		sqlite3_bind_int(s, 1 + ITEM_COLUMN_NONE, rowid);
	}

	bool success = true;

	if (sqlite3_step(s) != SQLITE_DONE) {
		success = false;
		if (rowid == -1) {
			FAIL("Item insertion failed: %s", db_error_string());
		} else {
			FAIL("Item update failed: %s", db_error_string());
		}
	}

	sqlite3_finalize(s);
	free_string(thumbnails_str);
	free_string(locations_str);
	free_string(categories_str);
	free_string(sources_str);
	free_string(attachments_str);
	free_string(authors_str);

	return success;
}

static inline void
reverse_linked_list_structures_of_item(struct getfeed_item *item)
{
	// Reverse linked lists to match the order in which the entries were added.
	// We don't have to reverse items list itself because in feed we already read
	// items in reverse order (from top (newest) to bottom (oldest)).
	reverse_person_list(&item->author);
	reverse_link_list(&item->attachment);
	reverse_source_list(&item->source);
	reverse_category_list(&item->category);
	reverse_string_list(&item->location);
	reverse_picture_list(&item->thumbnail);
}

bool
insert_item_data(const struct string *feed_url, struct getfeed_item *item)
{
	sqlite3_stmt *s = NULL;
	int step_status;

	// Before trying to write some item to the database we have to check if this
	// item is duplicate or not.

	if ((item->guid != NULL) && (item->guid->len != 0)) {
		if (db_prepare("SELECT rowid, summary, content FROM items WHERE feed_url = ? AND guid = ? LIMIT 1;", 83, &s) == false) {
			FAIL("Failed to prepare SELECT statement for searching item duplicate by guid!");
			return false;
		}
		db_bind_string(s, 1, feed_url);
		db_bind_string(s, 2, item->guid);
		step_status = sqlite3_step(s);
	} else if ((item->url != NULL) && (item->url->len != 0)) {
		if (db_prepare("SELECT rowid, summary, content FROM items WHERE feed_url = ? AND link = ? LIMIT 1;", 83, &s) == false) {
			FAIL("Failed to prepare SELECT statement for searching item duplicate by link!");
			return false;
		}
		db_bind_string(s, 1, feed_url);
		db_bind_string(s, 2, item->url);
		step_status = sqlite3_step(s);
	} else {
		step_status = SQLITE_DONE;
	}

	int item_rowid = -1;
	if (step_status == SQLITE_ROW) {
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
		item_rowid = sqlite3_column_int(s, 0);
	}

	if (s != NULL) {
		sqlite3_finalize(s);
	}

	reverse_linked_list_structures_of_item(item);

	return db_insert_item(feed_url, item, item_rowid);
}
