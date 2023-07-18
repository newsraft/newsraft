#include "newsraft.h"

static struct feed_entry **feeds;
static size_t feeds_count;
static struct items_list *items = NULL;
static pthread_mutex_t items_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile bool item_menu_needs_to_regenerate = false;

static struct format_arg fmt_args[] = {
	{L'n',  L"d", {.i = 0   }},
	{L'u',  L"c", {.c = '\0'}},
	{L'd',  L"s", {.s = NULL}},
	{L'l',  L"s", {.s = NULL}},
	{L't',  L"s", {.s = NULL}},
	{L'o',  L"s", {.s = NULL}},
	{L'L',  L"s", {.s = NULL}},
	{L'T',  L"s", {.s = NULL}},
	{L'O',  L"s", {.s = NULL}},
	{L'D',  L"s", {.s = NULL}},
	{L'\0', NULL, {.i = 0   }}, // terminator
};

bool
items_list_moderator(size_t index)
{
	if (items == NULL) {
		return false;
	}
	obtain_items_at_least_up_to_the_given_index(items, index);
	return index < items->len ? true : false;
}

const struct format_arg *
get_item_entry_args(size_t index)
{
	fmt_args[0].value.i = index + 1;
	fmt_args[1].value.c = items->ptr[index].is_unread == true ? 'N' : ' ';
	fmt_args[2].value.s = items->ptr[index].date_str->ptr;
	fmt_args[3].value.s = items->ptr[index].url->ptr;
	fmt_args[4].value.s = items->ptr[index].title ? items->ptr[index].title->ptr : "";
	fmt_args[5].value.s = items->ptr[index].title ? items->ptr[index].title->ptr : items->ptr[index].url->ptr;
	fmt_args[6].value.s = items->ptr[index].feed->link->ptr;
	fmt_args[7].value.s = items->ptr[index].feed->name ? items->ptr[index].feed->name->ptr : "";
	fmt_args[8].value.s = items->ptr[index].feed->name ? items->ptr[index].feed->name->ptr : items->ptr[index].feed->link->ptr;
	fmt_args[9].value.s = items->ptr[index].pub_date_str->ptr;
	return fmt_args;
}

int
paint_item_entry(size_t index)
{
	if (items->ptr[index].is_important == true) {
		return CFG_COLOR_LIST_ITEM_IMPORTANT_FG;
	} else if (items->ptr[index].is_unread == true) {
		return CFG_COLOR_LIST_ITEM_UNREAD_FG;
	} else {
		return CFG_COLOR_LIST_ITEM_FG;
	}
}

bool
unread_item_condition(size_t index)
{
	return items->ptr[index].is_unread;
}

bool
important_item_condition(size_t index)
{
	return items->ptr[index].is_important;
}

void
mark_item_read(size_t view_sel)
{
	if (items->ptr[view_sel].is_unread == true) {
		if (db_mark_item_read(items->ptr[view_sel].rowid) == true) {
			items->ptr[view_sel].is_unread = false;
			expose_entry_of_the_list_menu(view_sel);
		}
	}
}

void
mark_item_unread(size_t view_sel)
{
	if (items->ptr[view_sel].is_unread == false) {
		if (db_mark_item_unread(items->ptr[view_sel].rowid) == true) {
			items->ptr[view_sel].is_unread = true;
			expose_entry_of_the_list_menu(view_sel);
		}
	}
}

static void
mark_item_important(size_t view_sel)
{
	if (items->ptr[view_sel].is_important == false) {
		if (db_mark_item_important(items->ptr[view_sel].rowid) == true) {
			items->ptr[view_sel].is_important = true;
			expose_entry_of_the_list_menu(view_sel);
		}
	}
}

static void
mark_item_unimportant(size_t view_sel)
{
	if (items->ptr[view_sel].is_important == true) {
		if (db_mark_item_unimportant(items->ptr[view_sel].rowid) == true) {
			items->ptr[view_sel].is_important = false;
			expose_entry_of_the_list_menu(view_sel);
		}
	}
}

static void
mark_all_items_read(struct feed_entry **feeds, size_t feeds_count)
{
	if (db_mark_all_items_in_feeds_as_read(feeds, feeds_count) == true) {
		for (size_t i = 0; i < items->len; ++i) {
			items->ptr[i].is_unread = false;
		}
		expose_all_visible_entries_of_the_list_menu();
	}
}

static void
mark_all_items_unread(struct feed_entry **feeds, size_t feeds_count)
{
	if (db_mark_all_items_in_feeds_as_unread(feeds, feeds_count) == true) {
		for (size_t i = 0; i < items->len; ++i) {
			items->ptr[i].is_unread = true;
		}
		expose_all_visible_entries_of_the_list_menu();
	}
}

void
clean_up_items_menu(void)
{
	free_items_list(items);
}

void
tell_items_menu_to_regenerate(void)
{
	int8_t menu_type = get_current_menu_type();
	if (menu_type == ITEMS_MENU) {
		struct items_list *new_items = recreate_items_list(items);
		if (new_items != NULL) {
			pthread_mutex_lock(&items_lock);
			pthread_mutex_lock(&interface_lock);
			free_items_list(items);
			items = new_items;
			reset_list_menu_unprotected();
			pthread_mutex_unlock(&interface_lock);
			pthread_mutex_unlock(&items_lock);
		}
		item_menu_needs_to_regenerate = false;
	} else if (menu_type == PAGER_MENU) {
		item_menu_needs_to_regenerate = true;
	}
}

input_cmd_id
enter_items_menu_loop(struct feed_entry **new_feeds, size_t new_feeds_count, bool is_explore_mode)
{
	feeds = new_feeds;
	feeds_count = new_feeds_count;
	free_items_list(items);
	items = create_items_list(feeds, feeds_count, SORT_BY_TIME_DESC, get_cfg_bool(CFG_INITIAL_UNREAD_FIRST_SORTING));
	if (items == NULL) {
		// Error message written by create_items_list.
		return INPUT_ITEMS_MENU_WAS_NOT_CREATED;
	}

	const size_t *view_sel = enter_list_menu(ITEMS_MENU, is_explore_mode ? CFG_MENU_EXPLORE_ITEM_ENTRY_FORMAT : CFG_MENU_ITEM_ENTRY_FORMAT, true);

	input_cmd_id cmd;
	uint32_t count;
	const struct wstring *macro;
	while (true) {
		cmd = get_input_command(&count, &macro);
		pthread_mutex_lock(&items_lock);
		if (handle_list_menu_control(ITEMS_MENU, cmd, macro) == true) {
			// Rest a little.
		} else if (cmd == INPUT_MARK_READ_ALL) {
			mark_all_items_read(feeds, feeds_count);
		} else if (cmd == INPUT_MARK_UNREAD_ALL) {
			mark_all_items_unread(feeds, feeds_count);
		} else if (cmd == INPUT_MARK_IMPORTANT) {
			mark_item_important(*view_sel);
		} else if (cmd == INPUT_MARK_UNIMPORTANT) {
			mark_item_unimportant(*view_sel);
		} else if (cmd == INPUT_ENTER) {
			cmd = enter_item_pager_view_loop(items, view_sel);
			if (cmd == INPUT_QUIT_HARD) {
				break;
			}
			enter_list_menu(ITEMS_MENU, 0, false);
		} else if (cmd == INPUT_RELOAD) {
			update_feeds(&items->ptr[*view_sel].feed, 1);
		} else if (cmd == INPUT_RELOAD_ALL) {
			update_feeds(feeds, feeds_count);
		} else if (cmd == INPUT_SORT_NEXT) {
			change_sorting_order_of_items_list(&items, items->sort + 1);
		} else if (cmd == INPUT_SORT_PREV) {
			change_sorting_order_of_items_list(&items, items->sort - 1);
		} else if (cmd == INPUT_TOGGLE_UNREAD_FIRST_SORTING) {
			toggle_unread_first_sorting_of_items_list(&items);
		} else if (cmd == INPUT_STATUS_HISTORY_MENU) {
			cmd = enter_status_pager_view_loop();
			if (cmd == INPUT_QUIT_HARD) {
				break;
			}
			enter_list_menu(ITEMS_MENU, 0, false);
		} else if (cmd == INPUT_OPEN_IN_BROWSER) {
			run_command_with_specifiers(get_cfg_wstring(CFG_OPEN_IN_BROWSER_COMMAND), get_item_entry_args(*view_sel));
		} else if (cmd == INPUT_COPY_TO_CLIPBOARD) {
			copy_string_to_clipboard(items->ptr[*view_sel].url);
		} else if (cmd == INPUT_TOGGLE_EXPLORE_MODE && is_explore_mode == true) {
			break;
		} else if (cmd == INPUT_QUIT_SOFT || cmd == INPUT_QUIT_HARD) {
			break;
		}
		pthread_mutex_unlock(&items_lock);
		if (item_menu_needs_to_regenerate == true) {
			tell_items_menu_to_regenerate();
		}
	}
	// Since we exit the loop only through break, double unlock can't occur.
	pthread_mutex_unlock(&items_lock);

	// Get new feeds' unread counts before leaving list menu so that when we
	// calculate sections' unread counts in leave_list_menu() it gets the
	// actual data.
	int64_t new_unread_count;
	for (size_t i = 0; i < feeds_count; ++i) {
		new_unread_count = get_unread_items_count_of_the_feed(feeds[i]->link);
		if (new_unread_count >= 0) {
			feeds[i]->unread_count = new_unread_count;
		}
	}

	return cmd;
}
