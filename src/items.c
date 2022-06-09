#include <stdlib.h>
#include "newsraft.h"

static struct items_list *items;
static struct menu_list_settings items_menu;
static enum config_entry_index entry_format;

static struct format_arg fmt_args[] = {
	{L'n', L"d", {.i = 0}},
	{L'u', L"c", {.c = '\0'}},
	{L'f', L"s", {.s = NULL}},
	{L'd', L"s", {.s = NULL}},
	{L't', L"s", {.s = NULL}},
};

static const wchar_t *
paint_item_entry(size_t index)
{
	fmt_args[0].value.i = index + 1;
	fmt_args[1].value.c = items->list[index].is_unread == true ? 'N' : ' ';
	fmt_args[2].value.s = items->list[index].feed_name->ptr;
	fmt_args[3].value.s = items->list[index].date_str->ptr;
	fmt_args[4].value.s = items->list[index].title->ptr;
	return do_format(entry_format, fmt_args, COUNTOF(fmt_args));
}

static void
mark_item_read(size_t index)
{
	if (items->list[index].is_unread == false) {
		return; // success, item is already read
	}
	if (db_mark_item_read(items->list[index].rowid) == false) {
		return; // failure
	}
	items->list[index].is_unread = false;
	expose_entry_of_the_menu_list(&items_menu, index);
}

static void
mark_item_unread(size_t index)
{
	if (items->list[index].is_unread == true) {
		return; // success, item is already unread
	}
	if (db_mark_item_unread(items->list[index].rowid) == false) {
		return; // failure
	}
	items->list[index].is_unread = true;
	expose_entry_of_the_menu_list(&items_menu, index);
}

static void
mark_all_items_read(const struct feed_line **feeds, size_t feeds_count, struct menu_list_settings *menu)
{
	if (db_mark_all_items_in_feeds_as_read(feeds, feeds_count) == false) {
		return;
	}
	for (size_t i = 0; i < items->count; ++i) {
		if ((items->list[i].is_unread == true) && (i >= menu->view_min) && (i <= menu->view_max)) {
			items->list[i].is_unread = false;
			expose_entry_of_the_menu_list(menu, i);
		} else {
			items->list[i].is_unread = false;
		}
	}
}

static void
mark_all_items_unread(const struct feed_line **feeds, size_t feeds_count, struct menu_list_settings *menu)
{
	if (db_mark_all_items_in_feeds_as_unread(feeds, feeds_count) == false) {
		return;
	}
	for (size_t i = 0; i < items->count; ++i) {
		if ((items->list[i].is_unread == false) && (i >= menu->view_min) && (i <= menu->view_max)) {
			items->list[i].is_unread = true;
			expose_entry_of_the_menu_list(menu, i);
		} else {
			items->list[i].is_unread = true;
		}
	}
}

static inline void
initialize_menu_list_settings(struct menu_list_settings *menu)
{
	menu->entries_count = items->count;
	menu->view_sel = 0;
	menu->view_min = 0;
	menu->view_max = list_menu_height - 1;
	menu->paint_action = &paint_item_entry;
}

input_cmd_id
enter_items_menu_loop(const struct feed_line **feeds, size_t feeds_count, int format)
{
	items = generate_items_list(feeds, feeds_count, SORT_BY_TIME_DESC);
	if (items == NULL) {
		// Error message written by generate_items_list.
		return INPUTS_COUNT;
	}

	entry_format = format;

	initialize_menu_list_settings(&items_menu);

	status_clean();
	redraw_menu_list(&items_menu);

	uint32_t count;
	input_cmd_id cmd;
	while (true) {
		cmd = get_input_command(&count);
		if (cmd == INPUT_SELECT_NEXT) {
			list_menu_select_next(&items_menu);
			if (get_cfg_bool(CFG_MARK_ITEM_READ_ON_HOVER) == true) {
				mark_item_read(items_menu.view_sel);
			}
		} else if (cmd == INPUT_SELECT_PREV) {
			list_menu_select_prev(&items_menu);
			if (get_cfg_bool(CFG_MARK_ITEM_READ_ON_HOVER) == true) {
				mark_item_read(items_menu.view_sel);
			}
		} else if (cmd == INPUT_SELECT_NEXT_PAGE) {
			list_menu_select_next_page(&items_menu);
			if (get_cfg_bool(CFG_MARK_ITEM_READ_ON_HOVER) == true) {
				mark_item_read(items_menu.view_sel);
			}
		} else if (cmd == INPUT_SELECT_PREV_PAGE) {
			list_menu_select_prev_page(&items_menu);
			if (get_cfg_bool(CFG_MARK_ITEM_READ_ON_HOVER) == true) {
				mark_item_read(items_menu.view_sel);
			}
		} else if (cmd == INPUT_SELECT_FIRST) {
			list_menu_select_first(&items_menu);
			if (get_cfg_bool(CFG_MARK_ITEM_READ_ON_HOVER) == true) {
				mark_item_read(items_menu.view_sel);
			}
		} else if (cmd == INPUT_SELECT_LAST) {
			list_menu_select_last(&items_menu);
			if (get_cfg_bool(CFG_MARK_ITEM_READ_ON_HOVER) == true) {
				mark_item_read(items_menu.view_sel);
			}
		} else if (cmd == INPUT_MARK_READ) {
			mark_item_read(items_menu.view_sel);
		} else if (cmd == INPUT_MARK_UNREAD) {
			mark_item_unread(items_menu.view_sel);
		} else if (cmd == INPUT_MARK_READ_ALL) {
			mark_all_items_read(feeds, feeds_count, &items_menu);
		} else if (cmd == INPUT_MARK_UNREAD_ALL) {
			mark_all_items_unread(feeds, feeds_count, &items_menu);
		} else if (cmd == INPUT_ENTER) {
			cmd = enter_item_pager_view_loop(items->list[items_menu.view_sel].rowid);
			if (cmd == INPUT_QUIT_SOFT) {
				items->list[items_menu.view_sel].is_unread = 0;
				db_mark_item_read(items->list[items_menu.view_sel].rowid);
				redraw_menu_list(&items_menu);
			} else if (cmd == INPUT_QUIT_HARD) {
				break;
			}
		} else if (cmd == INPUT_STATUS_HISTORY_MENU) {
			cmd = enter_status_pager_view_loop();
			if (cmd == INPUT_QUIT_SOFT) {
				redraw_menu_list(&items_menu);
			} else if (cmd == INPUT_QUIT_HARD) {
				break;
			}
		} else if (cmd == INPUT_RESIZE) {
			redraw_menu_list(&items_menu);
		} else if ((cmd == INPUT_QUIT_SOFT) || (cmd == INPUT_QUIT_HARD)) {
			break;
		}
	}

	free_items_list(items);

	return cmd;
}
