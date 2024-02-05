#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

static inline bool
append_sorting_order_expression_to_query(struct string *query, int order)
{
	switch (order) {
		case SORT_BY_TIME_ASC:      return catas(query, " ORDER BY MAX(publication_date, update_date) ASC, rowid ASC", 59);
		case SORT_BY_TIME_DESC:     return catas(query, " ORDER BY MAX(publication_date, update_date) DESC, rowid DESC", 61);
		case SORT_BY_UNREAD_ASC:    return catas(query, " ORDER BY unread ASC, MAX(publication_date, update_date) DESC, rowid DESC", 73);
		case SORT_BY_UNREAD_DESC:   return catas(query, " ORDER BY unread DESC, MAX(publication_date, update_date) DESC, rowid DESC", 74);
		case SORT_BY_ALPHABET_ASC:  return catas(query, " ORDER BY title ASC, rowid ASC", 30);
		case SORT_BY_ALPHABET_DESC: return catas(query, " ORDER BY title DESC, rowid DESC", 32);
	}
	return false;
}

static inline struct string *
generate_search_query_string(const struct items_list *items, const struct string *search_filter)
{
	struct string *q = crtas("SELECT rowid,feed_url,title,link,publication_date,update_date,unread,important FROM items WHERE (feed_url=?", 107);
	if (q == NULL) goto error;
	for (size_t i = 1; i < items->feeds_count; ++i) {
		if (catas(q, " OR feed_url=?", 14) == false) goto error;
	}
	if (catcs(q, ')') == false) goto error;
	if (search_filter != NULL && search_filter->len > 0) {
		if (catas(q, " AND (title LIKE '%' || ? || '%')", 33) == false) goto error;
	}
	if (append_sorting_order_expression_to_query(q, items->sorting) == false) goto error;
	return q;
error:
	FAIL("Not enough memory for query string!");
	free_string(q);
	return NULL;
}

void
free_items_list(struct items_list *items)
{
	if (items != NULL) {
		for (size_t i = 0; i < items->len; ++i) {
			free_string(items->ptr[i].title);
			free_string(items->ptr[i].url);
			free_string(items->ptr[i].date_str);
			free_string(items->ptr[i].pub_date_str);
		}
		sqlite3_finalize(items->res);
		free_string(items->query);
		free_string(items->search_filter);
		free(items->ptr);
		free(items);
	}
}

static inline struct feed_entry **
find_feed_entry_by_url(struct feed_entry **feeds, size_t feeds_count, const char *feed_url)
{
	if (feed_url != NULL) {
		for (size_t i = 0; i < feeds_count; ++i) {
			if (strcmp(feed_url, feeds[i]->link->ptr) == 0) {
				return feeds + i;
			}
		}
	}
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
		items->ptr[items->len].pub_date_str = get_config_date_str(items->ptr[items->len].pub_date, CFG_LIST_ENTRY_DATE_FORMAT);

		items->ptr[items->len].is_unread = sqlite3_column_int(items->res, 6); // unread
		items->ptr[items->len].is_important = sqlite3_column_int(items->res, 7); // important

		items->len += 1;
	}
}

struct items_list *
create_items_list(struct feed_entry **feeds, size_t feeds_count, int sorting, const struct string *search_filter)
{
	INFO("Generating items list.");
	if (feeds_count == 0) {
		return NULL;
	}
	struct items_list *items = malloc(sizeof(struct items_list));
	if (items == NULL) {
		return NULL;
	}

	items->sorting = sorting;
	if (sorting < 0) {
		items->sorting = SORT_BY_TIME_DESC;
		const struct string *sort = get_cfg_string(CFG_MENU_ITEM_SORTING);
		if      (strcmp(sort->ptr, "time-desc")     == 0) items->sorting = SORT_BY_TIME_DESC;
		else if (strcmp(sort->ptr, "time-asc")      == 0) items->sorting = SORT_BY_TIME_ASC;
		else if (strcmp(sort->ptr, "unread-desc")   == 0) items->sorting = SORT_BY_UNREAD_DESC;
		else if (strcmp(sort->ptr, "unread-asc")    == 0) items->sorting = SORT_BY_UNREAD_ASC;
		else if (strcmp(sort->ptr, "alphabet-desc") == 0) items->sorting = SORT_BY_ALPHABET_DESC;
		else if (strcmp(sort->ptr, "alphabet-asc")  == 0) items->sorting = SORT_BY_ALPHABET_ASC;
	}

	items->ptr = NULL;
	items->len = 0;
	items->finished = false;
	items->feeds = feeds;
	items->feeds_count = feeds_count;
	items->search_filter = search_filter == NULL ? NULL : crtss(search_filter);
	items->query = generate_search_query_string(items, items->search_filter);
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
		db_bind_string(items->res, i + 1, feeds[i]->link);
	}
	if (items->search_filter != NULL && items->search_filter->len > 0) {
		db_bind_string(items->res, feeds_count + 1, items->search_filter);
	}
	obtain_items_at_least_up_to_the_given_index(items, 0);
	if (items->len < 1) {
		if (items->search_filter != NULL && items->search_filter->len > 0) {
			info_status("Items not found. Search query didn't get any matches!");
		} else {
			fail_status("Items not found. Make sure this feed is updated!");
		}
		goto undo3;
	}
	return items;
undo3:
	sqlite3_finalize(items->res);
undo2:
	free_string(items->query);
undo1:
	free_string(items->search_filter);
	free(items);
	return NULL;
}

bool
replace_items_list_with_empty_one(struct items_list **items)
{
	struct items_list *new_items = create_items_list((*items)->feeds, (*items)->feeds_count, (*items)->sorting, (*items)->search_filter);
	if (new_items == NULL) {
		return false;
	}
	pthread_mutex_lock(&interface_lock);
	free_items_list(*items);
	*items = new_items;
	reset_list_menu_unprotected();
	pthread_mutex_unlock(&interface_lock);
	return true;
}

void
change_items_list_sorting(struct items_list **items, input_cmd_id cmd)
{
	if (cmd == INPUT_SORT_BY_TIME) {
		(*items)->sorting = (*items)->sorting == SORT_BY_TIME_DESC ? SORT_BY_TIME_ASC : SORT_BY_TIME_DESC;
	} else if (cmd == INPUT_SORT_BY_UNREAD) {
		(*items)->sorting = (*items)->sorting == SORT_BY_UNREAD_DESC ? SORT_BY_UNREAD_ASC : SORT_BY_UNREAD_DESC;
	} else {
		(*items)->sorting = (*items)->sorting == SORT_BY_ALPHABET_ASC ? SORT_BY_ALPHABET_DESC : SORT_BY_ALPHABET_ASC;
	}
	replace_items_list_with_empty_one(items);

	const char *order = (*items)->sorting & 1 ? "descending" : "ascending";
	switch ((*items)->sorting & ~1) {
		case SORT_BY_TIME_ASC:     info_status("Sorted items by time (%s)", order); break;
		case SORT_BY_UNREAD_ASC:   info_status("Sorted items by unread (%s)", order); break;
		case SORT_BY_ALPHABET_ASC: info_status("Sorted items by alphabet (%s)", order); break;
	}
}

void
change_search_filter_of_items_list(struct items_list **items, const struct string *search_filter)
{
	struct string *prev_search_filter = (*items)->search_filter;
	(*items)->search_filter = crtss(search_filter);
	if (replace_items_list_with_empty_one(items) == true) {
		free_string(prev_search_filter);
	} else {
		free_string((*items)->search_filter);
		(*items)->search_filter = prev_search_filter;
	}
}
