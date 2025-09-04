#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

static inline bool
append_sorting_order_expression_to_query(struct string *q, int order)
{
	switch (order) {
		case SORT_BY_TIME_ASC:              catas(q, " ORDER BY MAX(publication_date, update_date) ASC, rowid ASC", 59); break;
		case SORT_BY_TIME_DESC:             catas(q, " ORDER BY MAX(publication_date, update_date) DESC, rowid DESC", 61); break;
		case SORT_BY_TIME_UPDATE_ASC:       catas(q, " ORDER BY update_date ASC, rowid ASC", 36); break;
		case SORT_BY_TIME_UPDATE_DESC:      catas(q, " ORDER BY update_date DESC, rowid DESC", 38); break;
		case SORT_BY_TIME_PUBLICATION_ASC:  catas(q, " ORDER BY publication_date ASC, rowid ASC", 41); break;
		case SORT_BY_TIME_PUBLICATION_DESC: catas(q, " ORDER BY publication_date DESC, rowid DESC", 43); break;
		case SORT_BY_ROWID_ASC:             catas(q, " ORDER BY rowid ASC", 19); break;
		case SORT_BY_ROWID_DESC:            catas(q, " ORDER BY rowid DESC", 20); break;
		case SORT_BY_UNREAD_ASC:            catas(q, " ORDER BY unread ASC, MAX(publication_date, update_date) DESC, rowid DESC", 73); break;
		case SORT_BY_UNREAD_DESC:           catas(q, " ORDER BY unread DESC, MAX(publication_date, update_date) DESC, rowid DESC", 74); break;
		case SORT_BY_IMPORTANT_ASC:         catas(q, " ORDER BY important ASC, MAX(publication_date, update_date) DESC, rowid DESC", 76); break;
		case SORT_BY_IMPORTANT_DESC:        catas(q, " ORDER BY important DESC, MAX(publication_date, update_date) DESC, rowid DESC", 77); break;
		case SORT_BY_ALPHABET_ASC:          catas(q, " ORDER BY title ASC, rowid ASC", 30); break;
		case SORT_BY_ALPHABET_DESC:         catas(q, " ORDER BY title DESC, rowid DESC", 32); break;
		default:
			fail_status("Unknown sorting method name!");
			return false;
	}
	return true;
}

static inline struct string *
generate_search_query_string(const struct menu_state *ctx)
{
	struct string *query = crtas("SELECT rowid,feed_url,guid,title,link,publication_date,update_date,unread,important FROM items WHERE ", 101);
	struct string *cond = generate_items_search_condition(ctx->items->feeds, ctx->items->feeds_count);
	if (!STRING_IS_EMPTY(cond)) {
		catss(query, cond);
	}
	free_string(cond);
	if (!STRING_IS_EMPTY(ctx->items->find_filter)) {
		catas(query, " AND (", 6);
		catss(query, ctx->items->find_filter);
		catcs(query, ')');
	}
	for (const struct menu_state *m = ctx; m != NULL; m = m->prev) {
		if (m->search_token) {
			catas(query, " AND ((title LIKE '%' || ? || '%') OR (content LIKE '%' || ? || '%'))", 69);
		}
	}
	if (append_sorting_order_expression_to_query(query, ctx->items->sorting) == false) {
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
		free_string(items->find_filter);
		free(items->ptr);
		free(items);
	}
}

static inline struct feed_entry **
find_feed_entry_by_url(struct feed_entry **feeds, size_t feeds_count, const char *feed_url)
{
	if (feed_url != NULL) {
		for (size_t i = 0; i < feeds_count; ++i) {
			if (strcmp(feed_url, feeds[i]->url->ptr) == 0) {
				return feeds + i;
			}
		}
	}
	return NULL;
}

void
obtain_items_at_least_up_to_the_given_index(struct items_list *items, sqlite3_stmt *force_res, size_t index)
{
	sqlite3_stmt *res = force_res ? force_res : items->res;
	if (items->finished == true) {
		return;
	}
	const char *text;
	int status;
	while (index >= items->len) {
		status = sqlite3_step(res);
		if (status != SQLITE_ROW) {
			items->finished = true;
			return;
		}

		items->ptr = newsraft_realloc(items->ptr, sizeof(struct item_entry) * (items->len + 1));

		items->ptr[items->len].rowid = sqlite3_column_int64(res, 0);

		text = (const char *)sqlite3_column_text(res, 1);
		items->ptr[items->len].feed = find_feed_entry_by_url(items->feeds, items->feeds_count, text);
		if (items->ptr[items->len].feed == NULL) {
			continue;
		}

		text = (const char *)sqlite3_column_text(res, 2);
		if (text == NULL) {
			continue;
		}
		items->ptr[items->len].guid = crtas(text, strlen(text));

		text = (const char *)sqlite3_column_text(res, 3);
		items->ptr[items->len].title = text == NULL ? crtes(1) : crtas(text, strlen(text));
		inlinefy_string(items->ptr[items->len].title);

		text = (const char *)sqlite3_column_text(res, 4);
		items->ptr[items->len].url = crtes(1);
		// Convert URL to absolute notation in case it's stored relative.
		//
		// For example, convert "/index.xml" to "http://example.org/index.xml"
		// We consider 2 base URLs for conversion to absolute notation:
		//
		// 1. link to webpage related to item's feed
		// 2. feed URL specified by the user
		//
		// If the first method doesn't yield a valid absolute notation, we try the second one.
		char *full_url = NULL;
		if (!STRING_IS_EMPTY(items->ptr[items->len].feed[0]->link)) {
			full_url = complete_url(items->ptr[items->len].feed[0]->link->ptr, text);
		}
		if (full_url == NULL) {
			full_url = complete_url(items->ptr[items->len].feed[0]->url->ptr, text);
		}
		if (full_url) {
			cpyas(&items->ptr[items->len].url, full_url, strlen(full_url));
			free(full_url);
		}

		items->ptr[items->len].pub_date = sqlite3_column_int64(res, 5);
		items->ptr[items->len].upd_date = sqlite3_column_int64(res, 6);
		if (items->ptr[items->len].pub_date <= 0 && items->ptr[items->len].upd_date > 0) {
			items->ptr[items->len].pub_date = items->ptr[items->len].upd_date;
		}
		if (items->ptr[items->len].upd_date <= 0 && items->ptr[items->len].pub_date > 0) {
			items->ptr[items->len].upd_date = items->ptr[items->len].pub_date;
		}
		items->ptr[items->len].date_str = get_cfg_date(NULL, CFG_LIST_ENTRY_DATE_FORMAT, items->ptr[items->len].upd_date);
		items->ptr[items->len].pub_date_str = get_cfg_date(NULL, CFG_LIST_ENTRY_DATE_FORMAT, items->ptr[items->len].pub_date);

		items->ptr[items->len].is_unread = sqlite3_column_int(res, 7); // unread
		items->ptr[items->len].is_important = sqlite3_column_int(res, 8); // important

		items->len += 1;
	}
}

bool
update_menu_item_list(struct menu_state *ctx)
{
	INFO("Updating menu's items list.");

	if (ctx->feeds_count == 0) {
		return false;
	}

	if (ctx->items == NULL) {
		INFO("Items list is empty - creating a new one.");
		ctx->items = newsraft_calloc(1, sizeof(*ctx->items));
		ctx->items->feeds = ctx->feeds_original;
		ctx->items->feeds_count = ctx->feeds_count;
		ctx->items->sorting = get_sorting_id(get_cfg_string(NULL, CFG_MENU_ITEM_SORTING)->ptr);
		ctx->items->find_filter = ctx->find_filter ? convert_wstring_to_string(ctx->find_filter) : NULL;
	}
	struct string *new_query = generate_search_query_string(ctx);
	if (new_query == NULL) {
		goto undo1;
	}
	sqlite3_stmt *new_res = db_prepare(new_query->ptr, new_query->len + 1, NULL);
	if (new_res == NULL) {
		fail_status("Can't run items search query! Make sure all item-rule settings are valid SQL conditions.");
		goto undo2;
	}
	for (size_t i = 0; i < ctx->items->feeds_count; ++i) {
		db_bind_string(new_res, i + 1, ctx->items->feeds[i]->url);
	}
	int search_token_iter = 0;
	for (const struct menu_state *m = ctx; m != NULL; m = m->prev) {
		if (m->search_token) {
			search_token_iter += 1;
			db_bind_string(new_res, ctx->items->feeds_count + search_token_iter, m->search_token);
			search_token_iter += 1;
			db_bind_string(new_res, ctx->items->feeds_count + search_token_iter, m->search_token);
		}
	}
	obtain_items_at_least_up_to_the_given_index(ctx->items, new_res, 0);
	if (ctx->items->len < 1) {
		if (search_token_iter > 0) {
			info_status("No items found. Search query didn't get any matches!");
		} else if (!STRING_IS_EMPTY(ctx->items->find_filter)) {
			info_status("No items found. Find query didn't get any matches!");
		} else {
			fail_status("No items found. Make sure this feed is updated!");
		}
		goto undo3;
	}
	pthread_mutex_lock(&interface_lock);
	sqlite3_finalize(ctx->items->res);
	free_string(ctx->items->query);
	ctx->items->res = new_res;
	ctx->items->query = new_query;
	reset_list_menu_unprotected();
	pthread_mutex_unlock(&interface_lock);
	return true;
undo3:
	sqlite3_finalize(new_res);
undo2:
	free_string(new_query);
undo1:
	free_string(ctx->items->find_filter);
	newsraft_free(ctx->items);
	ctx->items = NULL;
	return false;
}

void
change_items_list_sorting(struct menu_state *ctx, input_id cmd)
{
	static const struct { int primary; int secondary; } sort_map[] = {
		[INPUT_SORT_BY_TIME]             = {SORT_BY_TIME_DESC,             SORT_BY_TIME_ASC},
		[INPUT_SORT_BY_TIME_UPDATE]      = {SORT_BY_TIME_UPDATE_DESC,      SORT_BY_TIME_UPDATE_ASC},
		[INPUT_SORT_BY_TIME_PUBLICATION] = {SORT_BY_TIME_PUBLICATION_DESC, SORT_BY_TIME_PUBLICATION_ASC},
		[INPUT_SORT_BY_ROWID]            = {SORT_BY_ROWID_DESC,            SORT_BY_ROWID_ASC},
		[INPUT_SORT_BY_UNREAD]           = {SORT_BY_UNREAD_DESC,           SORT_BY_UNREAD_ASC},
		[INPUT_SORT_BY_ALPHABET]         = {SORT_BY_ALPHABET_ASC,          SORT_BY_ALPHABET_DESC},
		[INPUT_SORT_BY_IMPORTANT]        = {SORT_BY_IMPORTANT_DESC,        SORT_BY_IMPORTANT_ASC},
	};
	ctx->items->sorting = ctx->items->sorting == sort_map[cmd].primary ? sort_map[cmd].secondary : sort_map[cmd].primary;
	update_menu_item_list(ctx);
	info_status(get_sorting_message(ctx->items->sorting), "items");
}
