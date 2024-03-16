#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

static struct feed_entry **feeds = NULL;
static size_t feeds_count = 0;
static int feeds_sort = SORT_BY_ORIGINAL_ASC;
static struct feed_entry **feeds_original = NULL;

static bool
is_feed_valid(struct menu_state *ctx, size_t index)
{
	return index < ctx->feeds_count ? true : false;
}

static const struct format_arg *
get_feed_args(struct menu_state *ctx, size_t index)
{
	static struct format_arg feed_fmt[] = {
		{L'i',  L'd',  {.i = 0   }},
		{L'u',  L'd',  {.i = 0   }},
		{L'l',  L's',  {.s = NULL}},
		{L't',  L's',  {.s = NULL}},
		{L'\0', L'\0', {.i = 0   }}, // terminator
	};
	feed_fmt[0].value.i = index + 1;
	feed_fmt[1].value.i = ctx->feeds[index]->unread_count;
	feed_fmt[2].value.s = ctx->feeds[index]->link->ptr;
	feed_fmt[3].value.s = ctx->feeds[index]->name->ptr;
	return feed_fmt;
}

static int
paint_feed(struct menu_state *ctx, size_t index)
{
	return ctx->feeds[index]->unread_count > 0 ? CFG_COLOR_LIST_FEED_UNREAD : CFG_COLOR_LIST_FEED;
}

static bool
is_feed_unread(struct menu_state *ctx, size_t index)
{
	return ctx->feeds[index]->unread_count > 0;
}

static int
sort_feeds_original_comparison(const void *data1, const void *data2)
{
	struct feed_entry *feed1 = *(struct feed_entry **)data1;
	struct feed_entry *feed2 = *(struct feed_entry **)data2;
	size_t index1 = 0, index2 = 0;
	for (size_t i = 0; i < feeds_count; ++i) {
		if (feed1 == feeds_original[i]) index1 = i;
		if (feed2 == feeds_original[i]) index2 = i;
	}
	if (index1 > index2) return feeds_sort & 1 ? -1 : 1;
	if (index1 < index2) return feeds_sort & 1 ? 1 : -1;
	return 0;
}

static int
sort_feeds_by_unread_count_comparison(const void *data1, const void *data2)
{
	struct feed_entry *feed1 = *(struct feed_entry **)data1;
	struct feed_entry *feed2 = *(struct feed_entry **)data2;
	if (feed1->unread_count > feed2->unread_count) return feeds_sort & 1 ? -1 : 1;
	if (feed1->unread_count < feed2->unread_count) return feeds_sort & 1 ? 1 : -1;
	return sort_feeds_original_comparison(data1, data2) * (feeds_sort & 1 ? -1 : 1);
}

static int
sort_feeds_alphabetical_comparison(const void *data1, const void *data2)
{
	struct feed_entry *feed1 = *(struct feed_entry **)data1;
	struct feed_entry *feed2 = *(struct feed_entry **)data2;
	return strcmp(feed1->name->ptr, feed2->name->ptr) * (feeds_sort & 1 ? -1 : 1);
}

static inline void
sort_feeds(struct menu_state *m, int new_feeds_sort, bool we_are_already_in_feeds_menu)
{
	feeds          = m->feeds;
	feeds_count    = m->feeds_count;
	feeds_original = m->feeds_original;
	feeds_sort     = new_feeds_sort;
	const char *sort_msg = NULL;
	pthread_mutex_lock(&interface_lock);
	switch (feeds_sort & ~1) {
		case SORT_BY_ORIGINAL_ASC:
			sort_msg = "Sorted feeds by original order";
			qsort(feeds, feeds_count, sizeof(struct feed_entry *), &sort_feeds_original_comparison); break;
		case SORT_BY_UNREAD_ASC:
			sort_msg = "Sorted feeds by unread count";
			qsort(feeds, feeds_count, sizeof(struct feed_entry *), &sort_feeds_by_unread_count_comparison); break;
		case SORT_BY_ALPHABET_ASC:
			sort_msg = "Sorted feeds by alphabet";
			qsort(feeds, feeds_count, sizeof(struct feed_entry *), &sort_feeds_alphabetical_comparison); break;
	}
	pthread_mutex_unlock(&interface_lock);
	if (we_are_already_in_feeds_menu == true) {
		expose_all_visible_entries_of_the_list_menu();
		if (sort_msg != NULL) info_status("%s (%s)", sort_msg, feeds_sort & 1 ? "descending" : "ascending");
	}
}

struct menu_state *
feeds_menu_loop(struct menu_state *m)
{
	m->enumerator   = &is_feed_valid;
	m->get_args     = &get_feed_args;
	m->paint_action = &paint_feed;
	m->unread_state = &is_feed_unread;
	m->write_action = &list_menu_writer;
	m->entry_format = get_cfg_wstring(NULL, CFG_MENU_FEED_ENTRY_FORMAT);
	if (m->feeds_count < 1) {
		info_status("There are no feeds in this section");
		return setup_menu(NULL, NULL, 0, 0);
	} else if (!(m->flags & MENU_DISABLE_SETTINGS)) {
		if (get_cfg_bool(NULL, CFG_FEEDS_MENU_PARAMOUNT_EXPLORE) && get_items_count_of_feeds(m->feeds_original, m->feeds_count)) {
			return setup_menu(&items_menu_loop, m->feeds_original, m->feeds_count, MENU_IS_EXPLORE);
		} else if (m->feeds_count == 1 && get_items_count_of_feeds(m->feeds_original, m->feeds_count)) {
			return setup_menu(&items_menu_loop, m->feeds_original, m->feeds_count, MENU_SWALLOW);
		}
	}
	if (m->is_initialized == false) {
		m->feeds = malloc(sizeof(struct feed_entry *) * m->feeds_count);
		memcpy(m->feeds, m->feeds_original, sizeof(struct feed_entry *) * m->feeds_count);
		const struct string *sort = get_cfg_string(NULL, CFG_MENU_FEED_SORTING);
		if      (strcmp(sort->ptr, "unread-desc")   == 0) sort_feeds(m, SORT_BY_UNREAD_DESC,   false);
		else if (strcmp(sort->ptr, "unread-asc")    == 0) sort_feeds(m, SORT_BY_UNREAD_ASC,    false);
		else if (strcmp(sort->ptr, "alphabet-desc") == 0) sort_feeds(m, SORT_BY_ALPHABET_DESC, false);
		else if (strcmp(sort->ptr, "alphabet-asc")  == 0) sort_feeds(m, SORT_BY_ALPHABET_ASC,  false);
	}
	start_menu();
	const struct wstring *macro;
	for (input_cmd_id cmd = get_input_cmd(NULL, &macro) ;; cmd = get_input_cmd(NULL, &macro)) {
		if (handle_list_menu_control(m, cmd, macro) == true) {
			continue;
		}
		switch (cmd) {
			case INPUT_MARK_READ:       mark_feeds_read(m->feeds + m->view_sel, 1, true);  break;
			case INPUT_MARK_UNREAD:     mark_feeds_read(m->feeds + m->view_sel, 1, false); break;
			case INPUT_MARK_READ_ALL:   mark_feeds_read(m->feeds, m->feeds_count, true);   break;
			case INPUT_MARK_UNREAD_ALL: mark_feeds_read(m->feeds, m->feeds_count, false);  break;
			case INPUT_RELOAD:          update_feeds(m->feeds + m->view_sel, 1);           break;
			case INPUT_RELOAD_ALL:      update_feeds(m->feeds_original, m->feeds_count);   break;
			case INPUT_QUIT_HARD:       return NULL;
			case INPUT_ENTER:
				return setup_menu(&items_menu_loop, m->feeds + m->view_sel, 1, MENU_NO_FLAGS);
			case INPUT_TOGGLE_EXPLORE_MODE:
				return setup_menu(&items_menu_loop, m->feeds, m->feeds_count, MENU_IS_EXPLORE);
			case INPUT_APPLY_SEARCH_MODE_FILTER:
				return setup_menu(&items_menu_loop, m->feeds, m->feeds_count, MENU_IS_EXPLORE | MENU_USE_SEARCH);
			case INPUT_NAVIGATE_BACK:
				if (get_menu_depth() < 2) break;
				// fall through
			case INPUT_QUIT_SOFT:
				return setup_menu(NULL, NULL, 0, 0);
			case INPUT_SORT_BY_UNREAD:
				sort_feeds(m, feeds_sort == SORT_BY_UNREAD_DESC ? SORT_BY_UNREAD_ASC : SORT_BY_UNREAD_DESC, true);
				break;
			case INPUT_SORT_BY_ALPHABET:
				sort_feeds(m, feeds_sort == SORT_BY_ALPHABET_ASC ? SORT_BY_ALPHABET_DESC : SORT_BY_ALPHABET_ASC, true);
				break;
			case INPUT_STATUS_HISTORY_MENU:
				return setup_menu(&status_pager_loop, NULL, 0, MENU_NO_FLAGS);
		}
	}
	return NULL;
}
