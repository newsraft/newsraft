#include <string.h>
#include "newsraft.h"

bool
newsraft_json_parse(struct feed_update_state *data, const char *content, size_t content_size)
{
	sqlite3 *db;
	if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
		str_appendf(data->new_errors, "JSON parser failed: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_stmt *stmt;
	const char *sql = "SELECT key, path, value FROM json_tree(?) WHERE type != 'object' AND type != 'array';";
	if (sqlite3_prepare_v2(db, sql, strlen(sql) + 1, &stmt, 0) != SQLITE_OK) {
		str_appendf(data->new_errors, "JSON parser failed: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return false;
	}
	if (sqlite3_bind_text(stmt, 1, content, content_size, SQLITE_STATIC) != SQLITE_OK) {
		str_appendf(data->new_errors, "JSON parser failed: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		sqlite3_close(db);
		return false;
	}

	int64_t current_item_index = -1;
	int64_t current_author_index = -1;
	int64_t current_attachment_index = -1;
	long long dummy = -1;

	while (true) {
		int res = sqlite3_step(stmt);
		if (res == SQLITE_DONE) {
			break;
		} else if (res != SQLITE_ROW) {
			str_appendf(data->new_errors, "JSON parser failed: %s\n", sqlite3_errmsg(db));
			sqlite3_finalize(stmt);
			sqlite3_close(db);
			return false;
		}

		const char *key = (const char *)sqlite3_column_text(stmt, 0);
		const char *path = (const char *)sqlite3_column_text(stmt, 1);
		const char *value = (const char *)sqlite3_column_text(stmt, 2);
		INFO("%-30s|%-30s|%-30s\n", key ? key : "(null)", path ? path : "(null)", value ? value : "(null)");

		if (!key || !path || !value) {
			continue;
		}

		long long new_item_index = -1;
		long long new_author_index = -1;
		long long new_attachment_index = -1;

		if (strcmp(path, "$") == 0) {
			if (strcmp(key, "home_page_url") == 0) {
				cpyas(&data->feed.link, value, strlen(value));
			} else if (strcmp(key, "title") == 0) {
				cpyas(&data->feed.title, value, strlen(value));
			} else if (strcmp(key, "description") == 0) {
				serialize_caret(&data->feed.content);
				serialize_array(&data->feed.content, "text=", 5, value, strlen(value));
			}
			continue;
		}

		if (sscanf(path, "$.authors[%lld]", &new_author_index) == 1 && new_author_index >= 0) {
			if (new_author_index != current_author_index) {
				current_author_index = new_author_index;
				serialize_caret(&data->feed.persons);
				serialize_array(&data->feed.persons, "type=", 5, "author", 6);
			}
			if (strcmp(key, "name") == 0) {
				serialize_array(&data->feed.persons, "name=", 5, value, strlen(value));
			} else if (strcmp(key, "url") == 0) {
				serialize_array(&data->feed.persons, "url=", 4, value, strlen(value));
			}
			continue;
		}

		if (sscanf(path, "$.items[%lld]", &new_item_index) == 1 && new_item_index >= 0) {

			if (new_item_index != current_item_index) {
				current_item_index = new_item_index;
				current_author_index = -1;
				current_attachment_index = -1;
				prepend_item(&data->feed.item);
			}
			if (data->feed.item == NULL) {
				continue;
			}

			if (sscanf(path, "$.items[%lld].authors[%lld]", &dummy, &new_author_index) == 2 && new_author_index >= 0) {

				if (new_author_index != current_author_index) {
					current_author_index = new_author_index;
					serialize_caret(&data->feed.item->persons);
					serialize_array(&data->feed.item->persons, "type=", 5, "author", 6);
				}

				if (strcmp(key, "name") == 0) {
					serialize_array(&data->feed.item->persons, "name=", 5, value, strlen(value));
				} else if (strcmp(key, "url") == 0) {
					serialize_array(&data->feed.item->persons, "url=", 4, value, strlen(value));
				}

			} else if (sscanf(path, "$.items[%lld].attachments[%lld]", &dummy, &new_attachment_index) == 2 && new_attachment_index >= 0) {

				if (new_attachment_index != current_attachment_index) {
					current_attachment_index = new_attachment_index;
					serialize_caret(&data->feed.item->attachments);
				}

				if (strcmp(key, "url") == 0) {
					serialize_array(&data->feed.item->attachments, "url=", 4, value, strlen(value));
				} else if (strcmp(key, "mime_type") == 0) {
					serialize_array(&data->feed.item->attachments, "type=", 5, value, strlen(value));
				} else if (strcmp(key, "size_in_bytes") == 0) {
					serialize_array(&data->feed.item->attachments, "size=", 5, value, strlen(value));
				} else if (strcmp(key, "duration_in_seconds") == 0) {
					serialize_array(&data->feed.item->attachments, "duration=", 9, value, strlen(value));
				}

			} else {

				if (strcmp(key, "id") == 0) {
					cpyas(&data->feed.item->guid, value, strlen(value));
				} else if (strcmp(key, "url") == 0) {
					cpyas(&data->feed.item->link, value, strlen(value));
				} else if (strcmp(key, "title") == 0) {
					cpyas(&data->feed.item->title, value, strlen(value));
				} else if (strcmp(key, "content_html") == 0) {
					serialize_caret(&data->feed.item->content);
					serialize_array(&data->feed.item->content, "type=", 5, "text/html", 9);
					serialize_array(&data->feed.item->content, "text=", 5, value, strlen(value));
				} else if (strcmp(key, "content_text") == 0) {
					serialize_caret(&data->feed.item->content);
					serialize_array(&data->feed.item->content, "text=", 5, value, strlen(value));
				} else if (strcmp(key, "summary") == 0) {
					serialize_caret(&data->feed.item->content);
					serialize_array(&data->feed.item->content, "text=", 5, value, strlen(value));
				} else if (strcmp(key, "date_published") == 0) {
					data->feed.item->publication_date = parse_date_rfc3339(strlen(value) > 18 ? value : "");
				} else if (strcmp(key, "date_modified") == 0) {
					data->feed.item->update_date = parse_date_rfc3339(strlen(value) > 18 ? value : "");
				} else if (strcmp(key, "external_url") == 0) {
					serialize_caret(&data->feed.item->attachments);
					serialize_array(&data->feed.item->attachments, "url=", 4, value, strlen(value));
				} else if (strcmp(key, "tags") == 0) {
					serialize_caret(&data->feed.item->extras);
					serialize_array(&data->feed.item->extras, "category=", 9, value, strlen(value));
				}

			}
		}

	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return true;
}
