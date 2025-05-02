#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

static inline bool
append_sorting_order_expression_to_query(struct string *q, int order)
{
	switch (order) {
		case SORT_BY_TIME_ASC:       catas(q, " ORDER BY MAX(publication_date, update_date) ASC, rowid ASC", 59); break;
		case SORT_BY_TIME_DESC:      catas(q, " ORDER BY MAX(publication_date, update_date) DESC, rowid DESC", 61); break;
		case SORT_BY_ROWID_ASC:      catas(q, " ORDER BY rowid ASC", 19); break;
		case SORT_BY_ROWID_DESC:     catas(q, " ORDER BY rowid DESC", 20); break;
		case SORT_BY_UNREAD_ASC:     catas(q, " ORDER BY unread ASC, MAX(publication_date, update_date) DESC, rowid DESC", 73); break;
		case SORT_BY_UNREAD_DESC:    catas(q, " ORDER BY unread DESC, MAX(publication_date, update_date) DESC, rowid DESC", 74); break;
		case SORT_BY_IMPORTANT_ASC:  catas(q, " ORDER BY important ASC, MAX(publication_date, update_date) DESC, rowid DESC", 76); break;
		case SORT_BY_IMPORTANT_DESC: catas(q, " ORDER BY important DESC, MAX(publication_date, update_date) DESC, rowid DESC", 77); break;
		case SORT_BY_ALPHABET_ASC:   catas(q, " ORDER BY title ASC, rowid ASC", 30); break;
		case SORT_BY_ALPHABET_DESC:  catas(q, " ORDER BY title DESC, rowid DESC", 32); break;
		default:
			fail_status("Unknown sorting method name!");
			return false;
	}
	return true;
}

static inline struct string *
generate_search_query_string(const struct items_list *items, const struct string *search_filter)
{
	struct string *query = crtas("SELECT rowid,feed_url,guid,title,link,publication_date,update_date,unread,important FROM items WHERE ", 101);
	struct string *cond = generate_items_search_condition(items->feeds, items->feeds_count);
	if (!STRING_IS_EMPTY(cond)) {
		catss(query, cond);
	}
	free_string(cond);
	if (!STRING_IS_EMPTY(search_filter)) {
		catas(query, " AND ((title LIKE '%' || ? || '%') OR (content LIKE '%' || ? || '%'))", 69);
	}
	if (append_sorting_order_expression_to_query(query, items->sorting) == false) {
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
			free_string(items->ptr[i].guid);
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
	const char *text;
	int status;
	while (index >= items->len) {
		status = sqlite3_step(items->res);
		if (status != SQLITE_ROW) {
			items->finished = true;
			return;
		}

		items->ptr = newsraft_realloc(items->ptr, sizeof(struct item_entry) * (items->len + 1));

		items->ptr[items->len].rowid = sqlite3_column_int64(items->res, 0);

		text = (const char *)sqlite3_column_text(items->res, 1);
		items->ptr[items->len].feed = find_feed_entry_by_url(items->feeds, items->feeds_count, text);
		if (items->ptr[items->len].feed == NULL) {
			continue;
		}

		text = (const char *)sqlite3_column_text(items->res, 2);
		if (text == NULL) {
			continue;
		}
		items->ptr[items->len].guid = crtas(text, strlen(text));

		text = (const char *)sqlite3_column_text(items->res, 3);
		items->ptr[items->len].title = text == NULL ? crtes(1) : crtas(text, strlen(text));
		inlinefy_string(items->ptr[items->len].title);

		text = (const char *)sqlite3_column_text(items->res, 4);
		items->ptr[items->len].url = crtes(1);
		// Convert URL to absolute notation in case it's stored relative.
		// For example, convert "/index.xml" to "http://example.org/index.xml"
		char *full_url = complete_url(items->ptr[items->len].feed[0]->link->ptr, text);
		if (full_url != NULL) {
			cpyas(&items->ptr[items->len].url, full_url, strlen(full_url));
			free(full_url);
		}

		items->ptr[items->len].pub_date = sqlite3_column_int64(items->res, 5);
		items->ptr[items->len].upd_date = sqlite3_column_int64(items->res, 6);
		if (items->ptr[items->len].pub_date <= 0 && items->ptr[items->len].upd_date > 0) {
			items->ptr[items->len].pub_date = items->ptr[items->len].upd_date;
		}
		if (items->ptr[items->len].upd_date <= 0 && items->ptr[items->len].pub_date > 0) {
			items->ptr[items->len].upd_date = items->ptr[items->len].pub_date;
		}
		items->ptr[items->len].date_str = get_cfg_date(NULL, CFG_LIST_ENTRY_DATE_FORMAT, items->ptr[items->len].upd_date);
		items->ptr[items->len].pub_date_str = get_cfg_date(NULL, CFG_LIST_ENTRY_DATE_FORMAT, items->ptr[items->len].pub_date);

		items->ptr[items->len].is_unread = sqlite3_column_int(items->res, 7); // unread
		items->ptr[items->len].is_important = sqlite3_column_int(items->res, 8); // important

		items->len += 1;
	}
}

struct items_list *
create_items_list(struct feed_entry **feeds, size_t feeds_count, int sorting, const struct wstring *search_filter)
{
	INFO("Generating items list.");
	if (feeds_count == 0) {
		return NULL;
	}

	struct items_list *items = newsraft_calloc(1, sizeof(struct items_list));

	items->sorting = sorting;
	if (sorting < 0) {
		items->sorting = get_sorting_id(get_cfg_string(NULL, CFG_MENU_ITEM_SORTING)->ptr);
	}

	items->feeds = feeds;
	items->feeds_count = feeds_count;
	items->search_filter = search_filter == NULL ? NULL : convert_wstring_to_string(search_filter);
	items->query = generate_search_query_string(items, items->search_filter);
	if (items->query == NULL) {
		goto undo1;
	}
	items->res = db_prepare(items->query->ptr, items->query->len + 1, NULL);
	if (items->res == NULL) {
		fail_status("Can't run items search query! Make sure all item-rule settings are valid SQL conditions.");
		goto undo2;
	}
	for (size_t i = 0; i < feeds_count; ++i) {
		db_bind_string(items->res, i + 1, feeds[i]->link);
	}
	if (!STRING_IS_EMPTY(items->search_filter)) {
		db_bind_string(items->res, feeds_count + 1, items->search_filter);
		db_bind_string(items->res, feeds_count + 2, items->search_filter);
	}
	obtain_items_at_least_up_to_the_given_index(items, 0);
	if (items->len < 1) {
		if (!STRING_IS_EMPTY(items->search_filter)) {
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
recreate_items_list(struct items_list **items)
{
	struct wstring *filter = (*items)->search_filter ? convert_string_to_wstring((*items)->search_filter) : NULL;
	struct items_list *new_items = create_items_list((*items)->feeds, (*items)->feeds_count, (*items)->sorting, filter);
	free_wstring(filter);
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
change_items_list_sorting(struct items_list **items, input_id cmd)
{
	switch (cmd) {
		case INPUT_SORT_BY_TIME:
			(*items)->sorting = (*items)->sorting == SORT_BY_TIME_DESC ? SORT_BY_TIME_ASC : SORT_BY_TIME_DESC;
			break;
		case INPUT_SORT_BY_ROWID:
			(*items)->sorting = (*items)->sorting == SORT_BY_ROWID_DESC ? SORT_BY_ROWID_ASC : SORT_BY_ROWID_DESC;
			break;
		case INPUT_SORT_BY_UNREAD:
			(*items)->sorting = (*items)->sorting == SORT_BY_UNREAD_DESC ? SORT_BY_UNREAD_ASC : SORT_BY_UNREAD_DESC;
			break;
		case INPUT_SORT_BY_ALPHABET:
			(*items)->sorting = (*items)->sorting == SORT_BY_ALPHABET_ASC ? SORT_BY_ALPHABET_DESC : SORT_BY_ALPHABET_ASC;
			break;
		case INPUT_SORT_BY_IMPORTANT:
			(*items)->sorting = (*items)->sorting == SORT_BY_IMPORTANT_DESC ? SORT_BY_IMPORTANT_ASC : SORT_BY_IMPORTANT_DESC;
			break;
	}
	recreate_items_list(items);
	info_status(get_sorting_message((*items)->sorting), "items");
}

void
change_search_filter_of_items_list(struct items_list **items, const struct wstring *search_filter)
{
	struct string *prev_search_filter = (*items)->search_filter;
	(*items)->search_filter = convert_wstring_to_string(search_filter);
	if (recreate_items_list(items) == true) {
		free_string(prev_search_filter);
	} else {
		free_string((*items)->search_filter);
		(*items)->search_filter = prev_search_filter;
	}
}
