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
is_item_valid(size_t index)
{
	if (items == NULL) {
		return false;
	}
	obtain_items_at_least_up_to_the_given_index(items, index);
	return index < items->len ? true : false;
}

const struct format_arg *
get_item_args(size_t index)
{
	items_fmt_args[0].value.i = index + 1;
	items_fmt_args[1].value.s = items->ptr[index].is_unread == true ? "N" : " ";
	items_fmt_args[2].value.s = items->ptr[index].date_str->ptr;
	items_fmt_args[3].value.s = items->ptr[index].pub_date_str->ptr;
	items_fmt_args[4].value.s = items->ptr[index].url->ptr;
	items_fmt_args[5].value.s = items->ptr[index].title ? items->ptr[index].title->ptr : "";
	items_fmt_args[6].value.s = items->ptr[index].title ? items->ptr[index].title->ptr : items->ptr[index].url->ptr;
	items_fmt_args[7].value.s = items->ptr[index].feed[0]->link->ptr;
	items_fmt_args[8].value.s = items->ptr[index].feed[0]->name ? items->ptr[index].feed[0]->name->ptr : "";
	items_fmt_args[9].value.s = items->ptr[index].feed[0]->name ? items->ptr[index].feed[0]->name->ptr : items->ptr[index].feed[0]->link->ptr;
	return items_fmt_args;
}

int
paint_item(size_t index)
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
is_item_unread(size_t index)
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
	if (items->search_filter == NULL) {
		// Use intermediate variables to avoid race condition
		struct feed_entry **items_feeds = items->feeds;
		size_t items_feeds_count = items->feeds_count;
		pthread_mutex_unlock(&interface_lock);
		mark_feeds_read(items_feeds, items_feeds_count, status);
		pthread_mutex_lock(&interface_lock);
	} else {
		for (size_t i = 0; i < items->len; ++i) {
			if (db_mark_item_read(items->ptr[i].rowid, status) == true) {
				items->ptr[i].is_unread = false;
			}
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

struct menu_state *
items_menu_loop(struct menu_state *dest)
{
	bool need_reset = (items == NULL)
		|| (items->feeds != dest->feeds)
		|| (items->feeds_count != dest->feeds_count)
		|| (dest->flags & MENU_USE_SEARCH);
	if (need_reset) {
		items_menu_needs_to_regenerate = false;
		free_items_list(items);
		items = create_items_list(dest->feeds, dest->feeds_count, -1, dest->flags & MENU_USE_SEARCH ? search_mode_text_input : NULL);
		if (items == NULL) {
			return setup_menu(NULL, NULL, 0, MENU_DISABLE_SETTINGS); // Error displayed by create_items_list
		}
	}
	const size_t *view_sel = enter_list_menu(ITEMS_MENU, dest->flags & MENU_IS_EXPLORE ? CFG_MENU_EXPLORE_ITEM_ENTRY_FORMAT : CFG_MENU_ITEM_ENTRY_FORMAT, need_reset);
	const struct wstring *macro;
	while (true) {
		if (items_menu_needs_to_regenerate == true) {
			items_menu_needs_to_regenerate = false;
			replace_items_list_with_empty_one(&items);
		}
		if (get_cfg_bool(CFG_MARK_ITEM_READ_ON_HOVER) == true) {
			mark_item_read(*view_sel, true);
		}
		input_cmd_id cmd = get_input_cmd(NULL, &macro);
		if (handle_list_menu_control(ITEMS_MENU, cmd, macro) == true) {
			continue;
		}
		switch (cmd) {
			case INPUT_MARK_READ:         mark_item_read(*view_sel, true);                     break;
			case INPUT_MARK_UNREAD:       mark_item_read(*view_sel, false);                    break;
			case INPUT_MARK_READ_ALL:     mark_all_items_read(true);                           break;
			case INPUT_MARK_UNREAD_ALL:   mark_all_items_read(false);                          break;
			case INPUT_MARK_IMPORTANT:    mark_item_important(*view_sel);                      break;
			case INPUT_MARK_UNIMPORTANT:  mark_item_unimportant(*view_sel);                    break;
			case INPUT_RELOAD:            update_feeds(items->ptr[*view_sel].feed, 1);         break;
			case INPUT_RELOAD_ALL:        update_feeds(dest->feeds, dest->feeds_count);        break;
			case INPUT_COPY_TO_CLIPBOARD: copy_string_to_clipboard(items->ptr[*view_sel].url); break;
			case INPUT_QUIT_HARD:         return NULL;
			case INPUT_GOTO_FEED:
				if (dest->flags & MENU_IS_EXPLORE) return setup_menu(&items_menu_loop, items->ptr[*view_sel].feed, 1, MENU_NO_FLAGS);
				break;
			case INPUT_APPLY_SEARCH_MODE_FILTER:
				change_search_filter_of_items_list(&items, search_mode_text_input); break;
			case INPUT_OPEN_IN_BROWSER:
				run_formatted_command(get_cfg_wstring(CFG_OPEN_IN_BROWSER_COMMAND), get_item_args(*view_sel)); break;
			case INPUT_SORT_BY_TIME:
			case INPUT_SORT_BY_UNREAD:
			case INPUT_SORT_BY_ALPHABET:
				change_items_list_sorting(&items, cmd); break;
			case INPUT_ENTER:
				if (enter_item_pager_view_loop(items, view_sel) == INPUT_QUIT_HARD) return NULL;
				enter_list_menu(ITEMS_MENU, dest->flags & MENU_IS_EXPLORE ? CFG_MENU_EXPLORE_ITEM_ENTRY_FORMAT : CFG_MENU_ITEM_ENTRY_FORMAT, false);
				break;
			case INPUT_STATUS_HISTORY_MENU:
				if (enter_status_pager_view_loop() == INPUT_QUIT_HARD) return NULL;
				enter_list_menu(ITEMS_MENU, dest->flags & MENU_IS_EXPLORE ? CFG_MENU_EXPLORE_ITEM_ENTRY_FORMAT : CFG_MENU_ITEM_ENTRY_FORMAT, false);
				break;
		}
		if (cmd == INPUT_TOGGLE_EXPLORE_MODE && (dest->flags & MENU_IS_EXPLORE)) {
			if (dest->caller == &feeds_menu_loop) {
				return setup_menu(&feeds_menu_loop, dest->feeds, dest->feeds_count, MENU_SWALLOW | MENU_DISABLE_SETTINGS);
			} else {
				return setup_menu(&sections_menu_loop, NULL, 0, MENU_SWALLOW | MENU_DISABLE_SETTINGS);
			}
		} else if (cmd == INPUT_QUIT_SOFT || (cmd == INPUT_NAVIGATE_BACK && dest->caller != &sections_menu_loop)) {
			return setup_menu(NULL, NULL, 0, MENU_DISABLE_SETTINGS);
		}
	}
	return setup_menu(NULL, NULL, 0, MENU_DISABLE_SETTINGS);
}
