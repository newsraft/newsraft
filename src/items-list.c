#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

static inline bool
append_sorting_order_expression_to_query(struct string *query, enum sorting_order order)
{
	if (order == SORT_BY_NONE) {
		return true;
	} else if (order == SORT_BY_TIME_DESC) {
		return catas(query, " ORDER BY publication_date DESC, update_date DESC, rowid DESC", 61);
	} else if (order == SORT_BY_TIME_ASC) {
		return catas(query, " ORDER BY publication_date ASC, update_date ASC, rowid ASC", 58);
	} else if (order == SORT_BY_NAME_DESC) {
		return true; // TODO
	} else if (order == SORT_BY_NAME_ASC) {
		return true; // TODO
	}
	return true; // Will never happen.
}

static inline struct string *
generate_search_query_string(size_t feeds_count, enum sorting_order order)
{
	struct string *query = crtas("SELECT rowid,feed_url,title,link,publication_date,update_date,unread,important FROM items WHERE feed_url=?", 106);
	if (query == NULL) {
		FAIL("Not enough memory for query string!");
		return NULL;
	}
	for (size_t i = 1; i < feeds_count; ++i) {
		if (catas(query, " OR feed_url=?", 14) == false) {
			free_string(query);
			return NULL;
		}
	}
	if (append_sorting_order_expression_to_query(query, order) == false) {
		free_string(query);
		return NULL;
	}
	return query;
}

static inline struct items_list *
create_items_list(void)
{
	struct items_list *items = malloc(sizeof(struct items_list));
	if (items == NULL) {
		return NULL;
	}
	items->list = NULL;
	items->count = 0;
	return items;
}

void
free_items_list(struct items_list *items)
{
	if (items != NULL) {
		for (size_t i = 0; i < items->count; ++i) {
			free_string(items->list[i].title);
			free_string(items->list[i].url);
			free_string(items->list[i].date_str);
		}
		free(items->list);
		free(items);
	}
}

static inline const struct string *
find_feed_name_for_given_feed(struct feed_line **feeds, size_t feeds_count, const char *feed_url)
{
	for (size_t i = 0; i < feeds_count; ++i) {
		if (strcmp(feed_url, feeds[i]->link->ptr) == 0) {
			return feeds[i]->name != NULL ? feeds[i]->name : feeds[i]->link;
		}
	}
	return NULL;
}

struct items_list *
generate_items_list(struct feed_line **feeds, size_t feeds_count, enum sorting_order order)
{
	INFO("Generating items list.");
	if (feeds_count == 0) {
		FAIL("For some mysterious reason feeds_count is zero!");
		goto undo0;
	}
	struct string *query = generate_search_query_string(feeds_count, order);
	if (query == NULL) {
		fail_status("Can't generate search query string!");
		goto undo0;
	}
	sqlite3_stmt *res;
	if (db_prepare(query->ptr, query->len + 1, &res) == false) {
		fail_status("Can't prepare search query for action!");
		goto undo1;
	}
	for (size_t i = 0; i < feeds_count; ++i) {
		sqlite3_bind_text(res, i + 1, feeds[i]->link->ptr, feeds[i]->link->len, NULL);
	}
	struct items_list *items = create_items_list();
	if (items == NULL) {
		goto undo2;
	}
	void *tmp;
	const char *text;
	while (sqlite3_step(res) == SQLITE_ROW) {
		tmp = realloc(items->list, sizeof(struct item_entry) * (items->count + 1));
		if (tmp == NULL) {
			goto undo3;
		}
		items->list = tmp;

		items->list[items->count].rowid = sqlite3_column_int64(res, 0); // rowid

		text = (const char *)sqlite3_column_text(res, 1); // feed_url
		items->list[items->count].feed_name = find_feed_name_for_given_feed(feeds, feeds_count, text);

		items->list[items->count].title = db_get_plain_text_from_column(res, 2); // title
		if (items->list[items->count].title == NULL) {
			goto undo3;
		}

		text = (const char *)sqlite3_column_text(res, 3); // link
		if (text == NULL) {
			items->list[items->count].url = NULL;
		} else {
			items->list[items->count].url = crtas(text, strlen(text));
		}

		int64_t item_date = sqlite3_column_int64(res, 4); // publication_date
		int64_t tmp_date = sqlite3_column_int64(res, 5); // update_date
		if (tmp_date > item_date) {
			item_date = tmp_date;
		}
		items->list[items->count].date_str = get_config_date_str(item_date, CFG_LIST_ENTRY_DATE_FORMAT);

		items->list[items->count].is_unread = sqlite3_column_int(res, 6); // unread
		items->list[items->count].is_important = sqlite3_column_int(res, 7); // important

		items->count += 1;
	}
	if (items->count == 0) {
		info_status("Couldn't find any items in this feed.");
		goto undo3;
	}
	sqlite3_finalize(res);
	free_string(query);
	return items;
undo3:
	free_items_list(items);
undo2:
	sqlite3_finalize(res);
undo1:
	free_string(query);
undo0:
	return NULL;
}
