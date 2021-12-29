#include "feedeater.h"

static inline sqlite3_stmt *
find_item_by_its_rowid_in_db(int rowid)
{
	sqlite3_stmt *res;
	if (db_prepare("SELECT * FROM items WHERE rowid = ? LIMIT 1", -1, &res, NULL) != SQLITE_OK) {
		return NULL; // failure
	}
	sqlite3_bind_int(res, 1, rowid);
	if (sqlite3_step(res) != SQLITE_ROW) {
		WARN("Could not find an item with the rowid %d!", rowid);
		sqlite3_finalize(res);
		return NULL; // failure
	}
	INFO("Item with the rowid %d is found.", rowid);
	return res; // success
}

int
append_content(struct content_list **list, const char *content, size_t content_len, const char *content_type, size_t content_type_len)
{
	struct content_list *new_entry = malloc(sizeof(struct content_list));
	if (new_entry == NULL) {
		return 1;
	}
	new_entry->content = create_string(content, content_len);
	if (new_entry->content == NULL) {
		free(new_entry);
		return 1;
	}
	new_entry->content_type = malloc(sizeof(char) * (content_type_len + 1));
	if (new_entry->content_type == NULL) {
		free_string(new_entry->content);
		free(new_entry);
		return 1;
	}
	strncpy(new_entry->content_type, content_type, content_type_len);
	new_entry->content_type[content_type_len] = '\0';
	new_entry->next = NULL;

	if (*list != NULL) {
		struct content_list *temp = *list;
		while (temp != NULL) {
			if (temp->next == NULL) {
				temp->next = new_entry;
				break;
			}
			temp = temp->next;
		}
	} else {
		*list = new_entry;
	}
	return 0;
}

void
free_content_list(struct content_list *list)
{
	struct content_list *head_content = list;
	struct content_list *temp;
	while (head_content != NULL) {
		free_string(head_content->content);
		free(head_content->content_type);
		temp = head_content;
		head_content = head_content->next;
		free(temp);
	}
}

struct content_list *
create_content_list_for_item(int rowid)
{
	sqlite3_stmt *res = find_item_by_its_rowid_in_db(rowid);
	if (res == NULL) {
		return NULL;
	}

	struct content_list *list = NULL;

	if (append_meta_data_of_item(&list, res) != 0) {
		sqlite3_finalize(res);
		return NULL;
	}

	// Append data from content or summary column
	char *text = (char *)sqlite3_column_text(res, ITEM_COLUMN_CONTENT);
	char *text_type = (char *)sqlite3_column_text(res, ITEM_COLUMN_CONTENT_TYPE);
	size_t text_len = strlen(text);
	if (text_len == 0) {
		text = (char *)sqlite3_column_text(res, ITEM_COLUMN_SUMMARY);
		text_type = (char *)sqlite3_column_text(res, ITEM_COLUMN_SUMMARY_TYPE);
		text_len = strlen(text);
	}

	if (text_len != 0) {
		if (append_content(&list, text, text_len, text_type, strlen(text_type)) != 0) {
			free_content_list(list);
			sqlite3_finalize(res);
			return NULL;
		}
	}

	sqlite3_finalize(res);

	return list;
}
