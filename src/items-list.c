#include <stdlib.h>
#include "feedeater.h"

static inline bool
append_sorting_order_expression_to_query(struct string *query, enum sorting_order order)
{
	if (order == SORT_BY_NONE) {
		return true;
	} else if (order == SORT_BY_TIME_DESC) {
		return catas(query, " ORDER BY pubdate DESC, upddate DESC, rowid DESC", 48);
	} else if (order == SORT_BY_TIME_ASC) {
		return catas(query, " ORDER BY pubdate ASC, upddate ASC, rowid ASC", 45);
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
	struct string *query = crtas("SELECT rowid, title, unread, feed_url FROM items WHERE feed_url=?", 65);
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
	if (catcs(query, ';') == false) {
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
	if (items == NULL) {
		return;
	}
	for (size_t i = 0; i < items->count; ++i) {
		free_string(items->list[i].title);
	}
	free(items->list);
	free(items);
}

static inline const struct string *
find_feed_name_for_given_feed(const struct feed_line **feeds, size_t feeds_count, const char *feed_url)
{
	for (size_t i = 0; i < feeds_count; ++i) {
		if (strcmp(feed_url, feeds[i]->link->ptr) == 0) {
			return feeds[i]->name != NULL ? feeds[i]->name : feeds[i]->link;
		}
	}
	return NULL;
}

struct items_list *
generate_items_list(const struct feed_line **feeds, size_t feeds_count, enum sorting_order order)
{
	INFO("Generating items list.");
	if (feeds_count == 0) {
		FAIL("For some mysterious reason feeds_count is zero!");
		return NULL;
	}
	struct string *query = generate_search_query_string(feeds_count, order);
	if (query == NULL) {
		status_write("Can't generate search query string!");
		return NULL;
	}
	sqlite3_stmt *res;
	if (db_prepare(query->ptr, query->len + 1, &res, NULL) == false) {
		status_write("Can't prepare search query for action!");
		free_string(query);
		return NULL;
	}
	for (size_t i = 0; i < feeds_count; ++i) {
		sqlite3_bind_text(res, i + 1, feeds[i]->link->ptr, feeds[i]->link->len, NULL);
	}
	struct items_list *items = create_items_list();
	if (items == NULL) {
		free_string(query);
		sqlite3_finalize(res);
		return NULL;
	}
	void *tmp;
	const char *feed_url;
	while (sqlite3_step(res) == SQLITE_ROW) {
		tmp = realloc(items->list, sizeof(struct item_entry) * (items->count + 1));
		if (tmp == NULL) {
			free_string(query);
			sqlite3_finalize(res);
			free_items_list(items);
			return NULL;
		}
		items->list = tmp;
		items->list[items->count].rowid = sqlite3_column_int(res, 0);
		items->list[items->count].title = db_get_plain_text_from_column(res, 1);
		items->list[items->count].is_unread = sqlite3_column_int(res, 2);
		feed_url = (const char *)sqlite3_column_text(res, 3);
		items->list[items->count].feed_name = find_feed_name_for_given_feed(feeds, feeds_count, feed_url);
		if (items->list[items->count].title == NULL) {
			free_string(query);
			sqlite3_finalize(res);
			free_items_list(items);
			return NULL;
		}
		items->count += 1;
	}
	free_string(query);
	sqlite3_finalize(res);
	if ((items->list == NULL) || (items->count == 0)) {
		free_items_list(items);
		status_write("Couldn't find any items!");
		return NULL;
	}
	return items;
}
