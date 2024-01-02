#include <stdlib.h>
#include "newsraft.h"

enum { // Even is ascending, odd is descending
	FEEDS_SORT_BY_ORDER_ASC = 0,
	FEEDS_SORT_BY_ORDER_DESC,
	FEEDS_SORT_BY_UNREAD_COUNT_ASC,
	FEEDS_SORT_BY_UNREAD_COUNT_DESC,
	FEEDS_SORT_BY_ALPHABET_ASC,
	FEEDS_SORT_BY_ALPHABET_DESC,
	FEEDS_SORT_METHODS_COUNT,
};

static struct feed_entry **feeds = NULL;
static size_t feeds_count = 0;
static int feeds_sorting = FEEDS_SORT_BY_ORDER_ASC;
static struct feed_entry **original_feeds = NULL;

static struct format_arg feeds_fmt_args[] = {
	{L'i',  L'd',  {.i = 0   }},
	{L'u',  L'd',  {.i = 0   }},
	{L'l',  L's',  {.s = NULL}},
	{L't',  L's',  {.s = NULL}},
	{L'\0', L'\0', {.i = 0   }}, // terminator
};

bool
feeds_list_moderator(size_t index)
{
	return index < feeds_count ? true : false;
}

const struct format_arg *
get_feed_entry_args(size_t index)
{
	feeds_fmt_args[0].value.i = index + 1;
	feeds_fmt_args[1].value.i = feeds[index]->unread_count;
	feeds_fmt_args[2].value.s = feeds[index]->link->ptr;
	feeds_fmt_args[3].value.s = feeds[index]->name->ptr;
	return feeds_fmt_args;
}

int
paint_feed_entry(size_t index)
{
	return feeds[index]->unread_count > 0 ? CFG_COLOR_LIST_FEED_UNREAD_FG : CFG_COLOR_LIST_FEED_FG;
}

bool
unread_feed_condition(size_t index)
{
	return feeds[index]->unread_count > 0;
}

static int
feeds_sort_original_comparison(const void *data1, const void *data2)
{
	struct feed_entry *feed1 = *(struct feed_entry **)data1;
	struct feed_entry *feed2 = *(struct feed_entry **)data2;
	size_t index1 = 0, index2 = 0;
	for (size_t i = 0; i < feeds_count; ++i) {
		if (feed1 == original_feeds[i]) index1 = i;
		if (feed2 == original_feeds[i]) index2 = i;
	}
	if (index1 > index2) return feeds_sorting & 1 ? -1 : 1;
	if (index1 < index2) return feeds_sorting & 1 ? 1 : -1;
	return 0;
}

static int
feeds_sort_by_unread_count_comparison(const void *data1, const void *data2)
{
	struct feed_entry *feed1 = *(struct feed_entry **)data1;
	struct feed_entry *feed2 = *(struct feed_entry **)data2;
	if (feed1->unread_count > feed2->unread_count) return feeds_sorting & 1 ? -1 : 1;
	if (feed1->unread_count < feed2->unread_count) return feeds_sorting & 1 ? 1 : -1;
	return feeds_sort_original_comparison(data1, data2);
}

static int
feeds_sort_alphabetical_comparison(const void *data1, const void *data2)
{
	struct feed_entry *feed1 = *(struct feed_entry **)data1;
	struct feed_entry *feed2 = *(struct feed_entry **)data2;
	return strcmp(feed1->name->ptr, feed2->name->ptr) * (feeds_sorting & 1 ? -1 : 1);
}

static inline void
feeds_sort(int new_feeds_sorting, bool we_are_already_in_feeds_menu)
{
	feeds_sorting = new_feeds_sorting;
	const char *sort_msg = NULL;
	pthread_mutex_lock(&interface_lock);
	switch (feeds_sorting & ~1) {
		case FEEDS_SORT_BY_ORDER_ASC:
			sort_msg = "Sorted feeds by original order";
			qsort(feeds, feeds_count, sizeof(struct feed_entry *), &feeds_sort_original_comparison); break;
		case FEEDS_SORT_BY_UNREAD_COUNT_ASC:
			sort_msg = "Sorted feeds by unread count";
			qsort(feeds, feeds_count, sizeof(struct feed_entry *), &feeds_sort_by_unread_count_comparison); break;
		case FEEDS_SORT_BY_ALPHABET_ASC:
			sort_msg = "Sorted feeds by alphabet";
			qsort(feeds, feeds_count, sizeof(struct feed_entry *), &feeds_sort_alphabetical_comparison); break;
	}
	pthread_mutex_unlock(&interface_lock);
	if (we_are_already_in_feeds_menu == true) {
		expose_all_visible_entries_of_the_list_menu();
		if (sort_msg != NULL) info_status("%s (%s)", sort_msg, feeds_sorting & 1 ? "descending" : "ascending");
	}
}

static inline void
update_unread_items_count(size_t index)
{
	int64_t new_unread_count = get_unread_items_count_of_the_feed(feeds[index]->link);
	if (new_unread_count >= 0) {
		feeds[index]->unread_count = new_unread_count;
	}
}

static inline void
update_unread_items_count_of_all_feeds(void)
{
	for (size_t i = 0; i < feeds_count; ++i) {
		update_unread_items_count(i);
	}
}

void
mark_feed_read(size_t view_sel)
{
	db_mark_all_items_in_feeds_as_read(&feeds[view_sel], 1);
	update_unread_items_count(view_sel);
	expose_entry_of_the_list_menu(view_sel);
}

void
mark_feed_unread(size_t view_sel)
{
	db_mark_all_items_in_feeds_as_unread(&feeds[view_sel], 1);
	update_unread_items_count(view_sel);
	expose_entry_of_the_list_menu(view_sel);
}

static inline void
mark_all_feeds_read(void)
{
	db_mark_all_items_in_feeds_as_read(feeds, feeds_count);
	update_unread_items_count_of_all_feeds();
	expose_all_visible_entries_of_the_list_menu();
}

static inline void
mark_all_feeds_unread(void)
{
	db_mark_all_items_in_feeds_as_unread(feeds, feeds_count);
	update_unread_items_count_of_all_feeds();
	expose_all_visible_entries_of_the_list_menu();
}

input_cmd_id
enter_feeds_menu_loop(struct feed_entry **base_feeds, size_t base_feeds_count, struct feed_entry **feeds_view)
{
	memcpy(feeds_view, base_feeds, sizeof(struct feed_entry *) * base_feeds_count);
	feeds = feeds_view;
	feeds_count = base_feeds_count;
	original_feeds = base_feeds;
	feeds_sort(FEEDS_SORT_BY_ORDER_ASC, false);

	input_cmd_id cmd;
	if (get_cfg_bool(CFG_FEEDS_MENU_PARAMOUNT_EXPLORE) == true) {
		cmd = enter_items_menu_loop(feeds, feeds_count, true, NULL);
		if (cmd == INPUT_NAVIGATE_BACK || cmd == INPUT_QUIT_SOFT || cmd == INPUT_QUIT_HARD) {
			return cmd;
		}
	} else if (feeds_count == 1) {
		cmd = enter_items_menu_loop(feeds, 1, false, NULL);
		if (cmd != INPUT_ITEMS_MENU_WAS_NOT_CREATED) {
			return cmd;
		}
	}
	const size_t *view_sel = enter_list_menu(FEEDS_MENU, CFG_MENU_FEED_ENTRY_FORMAT, true);
	while (true) {
		const struct wstring *macro;
		cmd = get_input_command(NULL, &macro);
		if (handle_list_menu_control(FEEDS_MENU, cmd, macro) == true) {
			// Rest a little.
		} else if (cmd == INPUT_MARK_READ_ALL) {
			mark_all_feeds_read();
		} else if (cmd == INPUT_MARK_UNREAD_ALL) {
			mark_all_feeds_unread();
		} else if (cmd == INPUT_RELOAD) {
			update_feeds(feeds + *view_sel, 1);
		} else if (cmd == INPUT_RELOAD_ALL) {
			update_feeds(feeds, feeds_count);
		} else if (cmd == INPUT_ENTER) {
			cmd = enter_items_menu_loop(&feeds[*view_sel], 1, false, NULL);
			if (cmd == INPUT_QUIT_HARD) break;
			enter_list_menu(FEEDS_MENU, 0, false);
		} else if (cmd == INPUT_TOGGLE_EXPLORE_MODE) {
			cmd = enter_items_menu_loop(feeds, feeds_count, true, NULL);
			if (cmd == INPUT_QUIT_SOFT || cmd == INPUT_QUIT_HARD) break;
			enter_list_menu(FEEDS_MENU, 0, false);
		} else if (cmd == INPUT_APPLY_SEARCH_MODE_FILTER) {
			cmd = enter_items_menu_loop(feeds, feeds_count, true, search_mode_text_input);
			if (cmd == INPUT_QUIT_SOFT || cmd == INPUT_QUIT_HARD) break;
			enter_list_menu(FEEDS_MENU, 0, false);
		} else if (cmd == INPUT_SORT_NEXT) {
			feeds_sort((feeds_sorting + 2) % FEEDS_SORT_METHODS_COUNT, true);
		} else if (cmd == INPUT_SORT_PREV) {
			feeds_sort(feeds_sorting > 1 ? feeds_sorting - 2 : FEEDS_SORT_METHODS_COUNT - 2 + feeds_sorting % 2, true);
		} else if (cmd == INPUT_SORT_DIRECTION_TOGGLE) {
			feeds_sort(feeds_sorting ^ 1, true);
		} else if (cmd == INPUT_STATUS_HISTORY_MENU) {
			cmd = enter_status_pager_view_loop();
			if (cmd == INPUT_QUIT_HARD) break;
			enter_list_menu(FEEDS_MENU, 0, false);
		} else if (cmd == INPUT_NAVIGATE_BACK || cmd == INPUT_QUIT_SOFT || cmd == INPUT_QUIT_HARD) {
			break;
		}
	}
	return cmd;
}
