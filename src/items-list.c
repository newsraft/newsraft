#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

static inline bool
append_sorting_order_expression_to_query(struct string *query, sorting_order order)
{
	switch (order) {
		case SORT_BY_TIME_DESC:  return catas(query, " ORDER BY MAX(publication_date, update_date) DESC, rowid DESC", 61);
		case SORT_BY_TIME_ASC:   return catas(query, " ORDER BY MAX(publication_date, update_date) ASC, rowid ASC", 59);
		case SORT_BY_TITLE_DESC: return catas(query, " ORDER BY title DESC, rowid DESC", 32);
		case SORT_BY_TITLE_ASC:  return catas(query, " ORDER BY title ASC, rowid ASC", 30);
	}
	return false;
}

static inline struct string *
generate_search_query_string(size_t feeds_count, sorting_order order)
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

void
free_items_list(struct items_list *items)
{
	if (items != NULL) {
		for (size_t i = 0; i < items->len; ++i) {
			free_string(items->ptr[i].title);
			free_string(items->ptr[i].url);
			free_string(items->ptr[i].date_str);
		}
		sqlite3_finalize(items->res);
		free_string(items->query);
		free(items->ptr);
		free(items);
	}
}

static inline struct feed_entry *
find_feed_entry_by_url(struct feed_entry **feeds, size_t feeds_count, const char *feed_url)
{
	if (feed_url != NULL) {
		for (size_t i = 0; i < feeds_count; ++i) {
			if (strcmp(feed_url, feeds[i]->link->ptr) == 0) {
				return feeds[i];
			}
		}
	}
	return NULL;
}

struct items_list *
create_items_list(struct feed_entry **feeds, size_t feeds_count, sorting_order order)
{
	INFO("Generating items list.");
	if (feeds_count == 0) {
		return NULL;
	}
	struct items_list *items = malloc(sizeof(struct items_list));
	if (items == NULL) {
		return NULL;
	}
	items->sort = order;
	items->query = generate_search_query_string(feeds_count, items->sort);
	if (items->query == NULL) {
		fail_status("Can't generate search query string!");
		goto undo1;
	}
	items->res = db_prepare(items->query->ptr, items->query->len + 1);
	if (items->res == NULL) {
		fail_status("Can't prepare search query for action!");
		goto undo2;
	}
	for (size_t i = 0; i < feeds_count; ++i) {
		sqlite3_bind_text(items->res, i + 1, feeds[i]->link->ptr, feeds[i]->link->len, SQLITE_STATIC);
	}
	items->ptr = NULL;
	items->len = 0;
	items->finished = false;
	items->feeds = feeds;
	items->feeds_count = feeds_count;
	return items;
undo2:
	free_string(items->query);
undo1:
	free(items);
	return NULL;
}

void
obtain_items_at_least_up_to_the_given_index(struct items_list *items, size_t index)
{
	if (items->finished == true) {
		return;
	}
	void *tmp;
	const char *text;
	int status;
	while (index >= items->len) {
		status = sqlite3_step(items->res);
		if (status != SQLITE_ROW) {
			items->finished = true;
			return;
		}
		tmp = realloc(items->ptr, sizeof(struct item_entry) * (items->len + 1));
		if (tmp == NULL) {
			return;
		}
		items->ptr = tmp;

		items->ptr[items->len].rowid = sqlite3_column_int64(items->res, 0);

		text = (const char *)sqlite3_column_text(items->res, 1);
		items->ptr[items->len].feed = find_feed_entry_by_url(items->feeds, items->feeds_count, text);
		if (items->ptr[items->len].feed == NULL) {
			continue;
		}

		text = (const char *)sqlite3_column_text(items->res, 2);
		if (text == NULL) {
			items->ptr[items->len].title = NULL;
		} else {
			items->ptr[items->len].title = crtas(text, strlen(text));
			inlinefy_string(items->ptr[items->len].title);
		}

		text = (const char *)sqlite3_column_text(items->res, 3);
		items->ptr[items->len].url = text == NULL ? crtes(1) : crtas(text, strlen(text));

		items->ptr[items->len].pub_date = sqlite3_column_int64(items->res, 4);
		items->ptr[items->len].upd_date = sqlite3_column_int64(items->res, 5);
		int64_t max_date = items->ptr[items->len].pub_date;
		if (items->ptr[items->len].upd_date > max_date) {
			max_date = items->ptr[items->len].upd_date;
		}
		items->ptr[items->len].date_str = get_config_date_str(max_date, CFG_LIST_ENTRY_DATE_FORMAT);

		items->ptr[items->len].is_unread = sqlite3_column_int(items->res, 6); // unread
		items->ptr[items->len].is_important = sqlite3_column_int(items->res, 7); // important

		items->len += 1;
	}
}

bool
change_sorting_order_of_items_list(struct items_list **items, struct feed_entry **feeds, size_t feeds_count, sorting_order order)
{
	if (order > SORT_BY_TITLE_ASC) {
		order = SORT_BY_TIME_DESC;
	} else if (order < 0) {
		order = SORT_BY_TITLE_ASC;
	}
	struct items_list *new_items = create_items_list(feeds, feeds_count, order);
	if (new_items == NULL) {
		return false;
	}
	pthread_mutex_lock(&interface_lock);
	free_items_list(*items);
	*items = new_items;
	reset_list_menu_unprotected();
	pthread_mutex_unlock(&interface_lock);
	switch (order) {
		case SORT_BY_TIME_DESC:  info_status("Sorted items by time (descending).");  break;
		case SORT_BY_TIME_ASC:   info_status("Sorted items by time (ascending).");   break;
		case SORT_BY_TITLE_DESC: info_status("Sorted items by title (descending)."); break;
		case SORT_BY_TITLE_ASC:  info_status("Sorted items by title (ascending).");  break;
	}
	return true;
}
