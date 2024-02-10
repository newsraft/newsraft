#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

static struct feed_entry **feeds = NULL;
static size_t feeds_count = 0;
static int feeds_sort = SORT_BY_ORIGINAL_ASC;
static struct feed_entry **original_feeds = NULL;

static struct format_arg feeds_fmt_args[] = {
	{L'i',  L'd',  {.i = 0   }},
	{L'u',  L'd',  {.i = 0   }},
	{L'l',  L's',  {.s = NULL}},
	{L't',  L's',  {.s = NULL}},
	{L'\0', L'\0', {.i = 0   }}, // terminator
};

bool
is_feed_valid(size_t index)
{
	return index < feeds_count ? true : false;
}

const struct format_arg *
get_feed_args(size_t index)
{
	feeds_fmt_args[0].value.i = index + 1;
	feeds_fmt_args[1].value.i = feeds[index]->unread_count;
	feeds_fmt_args[2].value.s = feeds[index]->link->ptr;
	feeds_fmt_args[3].value.s = feeds[index]->name->ptr;
	return feeds_fmt_args;
}

int
paint_feed(size_t index)
{
	return feeds[index]->unread_count > 0 ? CFG_COLOR_LIST_FEED_UNREAD : CFG_COLOR_LIST_FEED;
}

bool
is_feed_unread(size_t index)
{
	return feeds[index]->unread_count > 0;
}

static int
sort_feeds_original_comparison(const void *data1, const void *data2)
{
	struct feed_entry *feed1 = *(struct feed_entry **)data1;
	struct feed_entry *feed2 = *(struct feed_entry **)data2;
	size_t index1 = 0, index2 = 0;
	for (size_t i = 0; i < feeds_count; ++i) {
		if (feed1 == original_feeds[i]) index1 = i;
		if (feed2 == original_feeds[i]) index2 = i;
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
sort_feeds(int new_feeds_sort, bool we_are_already_in_feeds_menu)
{
	feeds_sort = new_feeds_sort;
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
feeds_menu_loop(struct menu_state *dest)
{
	if (dest->feeds_count < 1) {
		info_status("There are no feeds in this section");
		return setup_menu(NULL, NULL, 0, 0);
	} else if (!(dest->flags & MENU_DISABLE_SETTINGS)) {
		if (get_cfg_bool(CFG_FEEDS_MENU_PARAMOUNT_EXPLORE) && get_items_count_of_feeds(dest->feeds, dest->feeds_count)) {
			return setup_menu(&items_menu_loop, dest->feeds, dest->feeds_count, MENU_IS_EXPLORE);
		} else if (dest->feeds_count == 1 && get_items_count_of_feeds(dest->feeds, dest->feeds_count)) {
			return setup_menu(&items_menu_loop, dest->feeds, dest->feeds_count, MENU_SWALLOW);
		}
	}
	bool need_reset = dest->feeds != original_feeds || dest->feeds_count != feeds_count;
	if (need_reset) {
		// We create a virtual/scratch feeds array to which we will apply sorting
		// while the original feeds will remain unchanged to preserve the order of
		// feeds that the user has set in the feeds file.
		pthread_mutex_lock(&interface_lock);
		free(feeds);
		feeds = malloc(sizeof(struct feed_entry *) * dest->feeds_count);
		memcpy(feeds, dest->feeds, sizeof(struct feed_entry *) * dest->feeds_count);
		feeds_count = dest->feeds_count;
		feeds_sort = SORT_BY_ORIGINAL_ASC;
		original_feeds = dest->feeds;
		pthread_mutex_unlock(&interface_lock);

		const struct string *sort = get_cfg_string(CFG_MENU_FEED_SORTING);
		if      (strcmp(sort->ptr, "unread-desc")   == 0) sort_feeds(SORT_BY_UNREAD_DESC, false);
		else if (strcmp(sort->ptr, "unread-asc")    == 0) sort_feeds(SORT_BY_UNREAD_ASC, false);
		else if (strcmp(sort->ptr, "alphabet-desc") == 0) sort_feeds(SORT_BY_ALPHABET_DESC, false);
		else if (strcmp(sort->ptr, "alphabet-asc")  == 0) sort_feeds(SORT_BY_ALPHABET_ASC, false);
	}
	const size_t *view_sel = enter_list_menu(FEEDS_MENU, CFG_MENU_FEED_ENTRY_FORMAT, need_reset);
	const struct wstring *macro;
	for (input_cmd_id cmd = get_input_cmd(NULL, &macro) ;; cmd = get_input_cmd(NULL, &macro)) {
		if (handle_list_menu_control(FEEDS_MENU, cmd, macro) == true) {
			continue;
		}
		switch (cmd) {
			case INPUT_MARK_READ:       mark_feeds_read(&feeds[*view_sel], 1, true);  break;
			case INPUT_MARK_UNREAD:     mark_feeds_read(&feeds[*view_sel], 1, false); break;
			case INPUT_MARK_READ_ALL:   mark_feeds_read(feeds, feeds_count, true);    break;
			case INPUT_MARK_UNREAD_ALL: mark_feeds_read(feeds, feeds_count, false);   break;
			case INPUT_RELOAD:          update_feeds(feeds + *view_sel, 1);           break;
			case INPUT_RELOAD_ALL:      update_feeds(feeds, feeds_count);             break;
			case INPUT_QUIT_HARD:       return NULL;
			case INPUT_ENTER:
				return setup_menu(&items_menu_loop, &dest->feeds[*view_sel], 1, MENU_NO_FLAGS);
			case INPUT_TOGGLE_EXPLORE_MODE:
				return setup_menu(&items_menu_loop, dest->feeds, dest->feeds_count, MENU_IS_EXPLORE);
			case INPUT_APPLY_SEARCH_MODE_FILTER:
				return setup_menu(&items_menu_loop, dest->feeds, dest->feeds_count, MENU_IS_EXPLORE | MENU_USE_SEARCH);
			case INPUT_NAVIGATE_BACK:
				if (dest->prev == NULL) break;
				// fall through
			case INPUT_QUIT_SOFT:
				return setup_menu(NULL, NULL, 0, 0);
			case INPUT_SORT_BY_UNREAD:
				sort_feeds(feeds_sort == SORT_BY_UNREAD_DESC ? SORT_BY_UNREAD_ASC : SORT_BY_UNREAD_DESC, true);
				break;
			case INPUT_SORT_BY_ALPHABET:
				sort_feeds(feeds_sort == SORT_BY_ALPHABET_ASC ? SORT_BY_ALPHABET_DESC : SORT_BY_ALPHABET_ASC, true);
				break;
			case INPUT_STATUS_HISTORY_MENU:
				if (enter_status_pager_view_loop() == INPUT_QUIT_HARD) return NULL;
				enter_list_menu(FEEDS_MENU, CFG_MENU_FEED_ENTRY_FORMAT, false);
				break;
		}
	}
	return NULL;
}
