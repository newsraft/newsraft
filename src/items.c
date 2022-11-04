#include "newsraft.h"

static struct items_list *items;

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
	{L'\0', NULL, {.i = 0   }}, // terminator
};

const struct format_arg *
prepare_item_entry_args(size_t index)
{
	fmt_args[0].value.i = index + 1;
	fmt_args[1].value.c = items->list[index].is_unread == true ? 'N' : ' ';
	fmt_args[2].value.s = items->list[index].date_str->ptr;
	fmt_args[3].value.s = items->list[index].url->ptr;
	fmt_args[4].value.s = items->list[index].title ? items->list[index].title->ptr : "";
	fmt_args[5].value.s = items->list[index].title ? items->list[index].title->ptr : items->list[index].url->ptr;
	fmt_args[6].value.s = items->list[index].feed->link->ptr;
	fmt_args[7].value.s = items->list[index].feed->name ? items->list[index].feed->name->ptr : "";
	fmt_args[8].value.s = items->list[index].feed->name ? items->list[index].feed->name->ptr : items->list[index].feed->link->ptr;
	return fmt_args;
}

int
paint_item_entry(size_t index)
{
	if (items->list[index].is_important == true) {
		return CFG_COLOR_LIST_ITEM_IMPORTANT_FG;
	} else if (items->list[index].is_unread == true) {
		return CFG_COLOR_LIST_ITEM_UNREAD_FG;
	} else {
		return CFG_COLOR_LIST_ITEM_FG;
	}
}

bool
unread_item_condition(size_t index)
{
	return items->list[index].is_unread;
}

bool
important_item_condition(size_t index)
{
	return items->list[index].is_important;
}

void
mark_selected_item_read(size_t view_sel)
{
	if (items->list[view_sel].is_unread == true) {
		if (db_mark_item_read(items->list[view_sel].rowid) == true) {
			items->list[view_sel].is_unread = false;
			expose_entry_of_the_list_menu(view_sel);
		}
	}
}

static void
mark_selected_item_unread(size_t view_sel)
{
	if (items->list[view_sel].is_unread == false) {
		if (db_mark_item_unread(items->list[view_sel].rowid) == true) {
			items->list[view_sel].is_unread = true;
			expose_entry_of_the_list_menu(view_sel);
		}
	}
}

static void
mark_selected_item_important(size_t view_sel)
{
	if (items->list[view_sel].is_important == false) {
		if (db_mark_item_important(items->list[view_sel].rowid) == true) {
			items->list[view_sel].is_important = true;
			expose_entry_of_the_list_menu(view_sel);
		}
	}
}

static void
mark_selected_item_unimportant(size_t view_sel)
{
	if (items->list[view_sel].is_important == true) {
		if (db_mark_item_unimportant(items->list[view_sel].rowid) == true) {
			items->list[view_sel].is_important = false;
			expose_entry_of_the_list_menu(view_sel);
		}
	}
}

static void
mark_all_items_read(struct feed_line **feeds, size_t feeds_count)
{
	if (db_mark_all_items_in_feeds_as_read(feeds, feeds_count) == true) {
		for (size_t i = 0; i < items->count; ++i) {
			items->list[i].is_unread = false;
		}
		expose_all_visible_entries_of_the_list_menu();
	}
}

static void
mark_all_items_unread(struct feed_line **feeds, size_t feeds_count)
{
	if (db_mark_all_items_in_feeds_as_unread(feeds, feeds_count) == true) {
		for (size_t i = 0; i < items->count; ++i) {
			items->list[i].is_unread = true;
		}
		expose_all_visible_entries_of_the_list_menu();
	}
}

input_cmd_id
enter_items_menu_loop(struct feed_line **feeds, size_t feeds_count, config_entry_id format_id)
{
	items = generate_items_list(feeds, feeds_count, SORT_BY_TIME_DESC);
	if (items == NULL) {
		// Error message written by generate_items_list.
		return INPUTS_COUNT;
	}

	const size_t *view_sel = enter_list_menu(ITEMS_MENU, items->count, format_id);

	input_cmd_id cmd;
	uint32_t count;
	const struct wstring *macro;
	while (true) {
		cmd = get_input_command(&count, &macro);
		if (handle_list_menu_navigation(cmd) == true) {
			// rest a little
		} else if (cmd == INPUT_MARK_READ) {
			mark_selected_item_read(*view_sel);
		} else if (cmd == INPUT_MARK_UNREAD) {
			mark_selected_item_unread(*view_sel);
		} else if (cmd == INPUT_MARK_READ_ALL) {
			mark_all_items_read(feeds, feeds_count);
		} else if (cmd == INPUT_MARK_UNREAD_ALL) {
			mark_all_items_unread(feeds, feeds_count);
		} else if (cmd == INPUT_MARK_IMPORTANT) {
			mark_selected_item_important(*view_sel);
		} else if (cmd == INPUT_MARK_UNIMPORTANT) {
			mark_selected_item_unimportant(*view_sel);
		} else if (cmd == INPUT_ENTER) {
			db_mark_item_read(items->list[*view_sel].rowid);
			items->list[*view_sel].is_unread = false;
			cmd = enter_item_pager_view_loop(items->list[*view_sel].rowid);
			if (cmd == INPUT_QUIT_HARD) {
				break;
			}
		} else if (cmd == INPUT_STATUS_HISTORY_MENU) {
			cmd = enter_status_pager_view_loop();
			if (cmd == INPUT_QUIT_HARD) {
				break;
			}
		} else if (cmd == INPUT_OPEN_IN_BROWSER) {
			open_url_in_browser(items->list[*view_sel].url);
		} else if (cmd == INPUT_COPY_TO_CLIPBOARD) {
			copy_string_to_clipboard(items->list[*view_sel].url);
		} else if ((cmd == INPUT_QUIT_SOFT) || (cmd == INPUT_QUIT_HARD)) {
			break;
		} else if ((cmd == INPUT_SYSTEM_COMMAND) && (macro != NULL)) {
			execute_command_with_specifiers_in_it(macro, prepare_item_entry_args(*view_sel));
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

	leave_list_menu(); // Leave before freeing to avoid potential use-after-frees.
	free_items_list(items);

	return cmd;
}
