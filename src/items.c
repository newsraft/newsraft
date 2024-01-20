#include "newsraft.h"

static struct items_list *items = NULL;
static volatile bool items_menu_needs_to_regenerate = false;

static struct format_arg items_fmt_args[] = {
	{L'i',  L'd',  {.i = 0   }},
	{L'u',  L's',  {.s = NULL}},
	{L'd',  L's',  {.s = NULL}},
	{L'D',  L's',  {.s = NULL}},
	{L'l',  L's',  {.s = NULL}},
	{L't',  L's',  {.s = NULL}},
	{L'o',  L's',  {.s = NULL}},
	{L'L',  L's',  {.s = NULL}},
	{L'T',  L's',  {.s = NULL}},
	{L'O',  L's',  {.s = NULL}},
	{L'\0', L'\0', {.i = 0   }}, // terminator
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
	items_fmt_args[0].value.i = index + 1;
	items_fmt_args[1].value.s = items->ptr[index].is_unread == true ? "N" : " ";
	items_fmt_args[2].value.s = items->ptr[index].date_str->ptr;
	items_fmt_args[3].value.s = items->ptr[index].pub_date_str->ptr;
	items_fmt_args[4].value.s = items->ptr[index].url->ptr;
	items_fmt_args[5].value.s = items->ptr[index].title ? items->ptr[index].title->ptr : "";
	items_fmt_args[6].value.s = items->ptr[index].title ? items->ptr[index].title->ptr : items->ptr[index].url->ptr;
	items_fmt_args[7].value.s = items->ptr[index].feed->link->ptr;
	items_fmt_args[8].value.s = items->ptr[index].feed->name ? items->ptr[index].feed->name->ptr : "";
	items_fmt_args[9].value.s = items->ptr[index].feed->name ? items->ptr[index].feed->name->ptr : items->ptr[index].feed->link->ptr;
	return items_fmt_args;
}

int
paint_item_entry(size_t index)
{
	if (items->ptr[index].is_important == true) {
		return CFG_COLOR_LIST_ITEM_IMPORTANT;
	} else if (items->ptr[index].is_unread == true) {
		return CFG_COLOR_LIST_ITEM_UNREAD;
	} else {
		return CFG_COLOR_LIST_ITEM;
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

static void
mark_item_read(size_t view_sel, bool status)
{
	if (items->ptr[view_sel].is_unread == status) {
		if (db_mark_item_read(items->ptr[view_sel].rowid, status) == true) {
			items->ptr[view_sel].is_unread = !status;
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
mark_all_items_read(bool status)
{
	pthread_mutex_lock(&interface_lock);
	for (size_t i = 0; i < items->len; ++i) {
		if (db_mark_item_read(items->ptr[i].rowid, status) == true) {
			items->ptr[i].is_unread = false;
		}
	}
	pthread_mutex_unlock(&interface_lock);
	expose_all_visible_entries_of_the_list_menu();
}

void
clean_up_items_menu(void)
{
	free_items_list(items);
}

void
tell_items_menu_to_regenerate(void)
{
	items_menu_needs_to_regenerate = true;
	break_getting_input_command();
}

input_cmd_id
enter_items_menu_loop(struct feed_entry **feeds, size_t feeds_count, bool is_explore_mode, const struct string *search_filter)
{
	items_menu_needs_to_regenerate = false;
	free_items_list(items);
	items = create_items_list(feeds, feeds_count, -1, search_filter);
	if (items == NULL) {
		// Error message written by create_items_list.
		return INPUT_ITEMS_MENU_WAS_NOT_CREATED;
	}

	const size_t *view_sel = enter_list_menu(ITEMS_MENU, is_explore_mode ? CFG_MENU_EXPLORE_ITEM_ENTRY_FORMAT : CFG_MENU_ITEM_ENTRY_FORMAT, true);

	input_cmd_id cmd;
	const struct wstring *macro;
	while (true) {
		if (items_menu_needs_to_regenerate == true) {
			items_menu_needs_to_regenerate = false;
			replace_items_list_with_empty_one(&items);
		}
		if (get_cfg_bool(CFG_MARK_ITEM_READ_ON_HOVER) == true) {
			mark_item_read(*view_sel, true);
		}
		cmd = get_input_command(NULL, &macro);
		if (handle_list_menu_control(ITEMS_MENU, cmd, macro) == true) {
			// Rest a little.
		} else if (cmd == INPUT_MARK_READ) {
			mark_item_read(*view_sel, true);
		} else if (cmd == INPUT_MARK_UNREAD) {
			mark_item_read(*view_sel, false);
		} else if (cmd == INPUT_MARK_READ_ALL) {
			mark_all_items_read(true);
		} else if (cmd == INPUT_MARK_UNREAD_ALL) {
			mark_all_items_read(false);
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
		} else if (cmd == INPUT_APPLY_SEARCH_MODE_FILTER) {
			change_search_filter_of_items_list(&items, search_mode_text_input);
		} else if (cmd == INPUT_SORT_BY_TIME || cmd == INPUT_SORT_BY_UNREAD || cmd == INPUT_SORT_BY_ALPHABET) {
			change_items_list_sorting(&items, cmd);
		} else if (cmd == INPUT_STATUS_HISTORY_MENU) {
			cmd = enter_status_pager_view_loop();
			if (cmd == INPUT_QUIT_HARD) {
				break;
			}
			enter_list_menu(ITEMS_MENU, 0, false);
		} else if (cmd == INPUT_OPEN_IN_BROWSER) {
			run_formatted_command(get_cfg_wstring(CFG_OPEN_IN_BROWSER_COMMAND), get_item_entry_args(*view_sel));
		} else if (cmd == INPUT_COPY_TO_CLIPBOARD) {
			copy_string_to_clipboard(items->ptr[*view_sel].url);
		} else if (cmd == INPUT_TOGGLE_EXPLORE_MODE && is_explore_mode == true) {
			break;
		} else if (cmd == INPUT_NAVIGATE_BACK || cmd == INPUT_QUIT_SOFT || cmd == INPUT_QUIT_HARD) {
			break;
		}
	}

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
