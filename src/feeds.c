#include <stdlib.h>
#include <stdio.h>
#include "newsraft.h"

static struct feed_line **feeds = NULL;
static size_t feeds_count = 0;

static struct menu_list_settings feeds_menu;

static struct format_arg fmt_args[] = {
	{L'n',  L"d", {.i = 0}},
	{L'u',  L"d", {.i = 0}},
	{L'l',  L"s", {.s = NULL}},
	{L't',  L"s", {.s = NULL}},
	{L'o',  L"s", {.s = NULL}},
	{L'\0', NULL, {.i = 0}}, // terminator
};

bool
load_feeds(void)
{
	const char *feeds_file_path = get_feeds_path();
	if (feeds_file_path == NULL) {
		// Error message written by get_feeds_path.
		return false;
	}
	if (create_global_section() == false) {
		fputs("Not enough memory for global section structure!\n", stderr);
		return false;
	}
	if (parse_feeds_file(feeds_file_path) == false) {
		fputs("Failed to load feeds from file!\n", stderr);
		return false;
	}

	// Display feeds of global section (that is all feeds) by default.
	obtain_feeds_of_global_section(&feeds, &feeds_count);

	if (feeds_count == 0) {
		fputs("Not a single feed was loaded!\n", stderr);
		return false;
	}

	for (size_t i = 0; i < feeds_count; ++i) {
		feeds[i]->unread_count = get_unread_items_count_of_the_feed(feeds[i]->link);
		if (feeds[i]->unread_count < 0) {
			fprintf(stderr, "Failed to get unread items count of the \"%s\" feed!\n", feeds[i]->link->ptr);
			return false;
		}
	}

	return true;
}

static const wchar_t *
write_feed_entry(size_t index)
{
	fmt_args[0].value.i = index + 1;
	fmt_args[1].value.i = feeds[index]->unread_count;
	fmt_args[2].value.s = feeds[index]->link ? feeds[index]->link->ptr : "";
	fmt_args[3].value.s = feeds[index]->name ? feeds[index]->name->ptr : "";
	if (feeds[index]->name != NULL) {
		fmt_args[4].value.s = feeds[index]->name->ptr;
	} else {
		if (feeds[index]->link != NULL) {
			fmt_args[4].value.s = feeds[index]->link->ptr;
		} else {
			fmt_args[4].value.s = "";
		}
	}
	return do_format(get_cfg_wstring(CFG_MENU_FEED_ENTRY_FORMAT), fmt_args);
}

static int
paint_feed_entry(size_t index)
{
	if (feeds[index]->unread_count > 0) {
		return CFG_COLOR_LIST_FEED_UNREAD_FG;
	} else {
		return CFG_COLOR_LIST_FEED_FG;
	}
}

static void
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

static inline void
mark_selected_feed_read(void)
{
	db_mark_all_items_in_feeds_as_read(&feeds[feeds_menu.view_sel], 1);
	update_unread_items_count(feeds_menu.view_sel);
	expose_entry_of_the_menu_list(&feeds_menu, feeds_menu.view_sel);
}

static inline void
mark_selected_feed_unread(void)
{
	db_mark_all_items_in_feeds_as_unread(&feeds[feeds_menu.view_sel], 1);
	update_unread_items_count(feeds_menu.view_sel);
	expose_entry_of_the_menu_list(&feeds_menu, feeds_menu.view_sel);
}

static inline void
mark_all_feeds_read(void)
{
	db_mark_all_items_in_feeds_as_read(feeds, feeds_count);
	update_unread_items_count_of_all_feeds();
	expose_all_visible_entries_of_the_menu_list(&feeds_menu);
}

static inline void
mark_all_feeds_unread(void)
{
	db_mark_all_items_in_feeds_as_unread(feeds, feeds_count);
	update_unread_items_count_of_all_feeds();
	expose_all_visible_entries_of_the_menu_list(&feeds_menu);
}

bool
update_and_refresh_feed(struct feed_line *feed)
{
	int8_t status = update_feed(feed->link);
	if (status == DOWNLOAD_SUCCEEDED) {
		int64_t new_unread_count = get_unread_items_count_of_the_feed(feed->link);
		if (new_unread_count < 0) {
			return false;
		}
		feed->unread_count = new_unread_count;
		if ((feed->name == NULL) || (feed->name->len == 0)) {
			struct string *title = db_get_string_from_feed_table(feed->link, "title", 5);
			if (title != NULL) {
				inlinify_string(title);
				crtss_or_cpyss(&feed->name, title);
				free_string(title);
			}
		}
	} else if (status == DOWNLOAD_FAILED) {
		return false;
	}
	return true;
}

static void
reload_current_feed(void)
{
	info_status("Loading %s", feeds[feeds_menu.view_sel]->link->ptr);

	if (update_and_refresh_feed(feeds[feeds_menu.view_sel]) == false) {
		fail_status("Failed to update %s", feeds[feeds_menu.view_sel]->link->ptr);
		return;
	}

	expose_entry_of_the_menu_list(&feeds_menu, feeds_menu.view_sel);
	status_clean();
}

static void
reload_all_feeds(void)
{
	const char *failed_url;
	size_t errors = 0;

	for (size_t i = 0; i < feeds_count; ++i) {
		info_status("(%zu/%zu) Loading %s", i + 1, feeds_count, feeds[i]->link->ptr);
		if (update_and_refresh_feed(feeds[i]) == true) {
			expose_entry_of_the_menu_list(&feeds_menu, i);
		} else {
			failed_url = feeds[i]->link->ptr;
			fail_status("Failed to update %s", failed_url);
			++errors;
		}
	}

	if (errors == 0) {
		status_clean();
	} else if (errors == 1) {
		fail_status("Failed to update %s", failed_url);
	} else {
		fail_status("Failed to update %zu feeds (check out status history for more details)", errors);
	}
}

static bool
unread_feed_condition(size_t index)
{
	return (feeds[index]->unread_count > 0) ? true : false;
}

input_cmd_id
enter_feeds_menu_loop(struct feed_line **new_feeds, size_t new_feeds_count)
{
	feeds = new_feeds;
	feeds_count = new_feeds_count;
	feeds_menu.write_action = &write_feed_entry;
	feeds_menu.paint_action = &paint_feed_entry;
	feeds_menu.hover_action = NULL;
	feeds_menu.unread_condition = &unread_feed_condition;
	reset_menu_list_settings(&feeds_menu, feeds_count);

	redraw_menu_list(&feeds_menu);

	input_cmd_id cmd;
	uint32_t count;
	const struct wstring *macro;
	while (true) {
		cmd = get_input_command(&count, &macro);
		if (cmd == INPUT_SELECT_NEXT) {
			list_menu_select_next(&feeds_menu);
		} else if (cmd == INPUT_SELECT_PREV) {
			list_menu_select_prev(&feeds_menu);
		} else if (cmd == INPUT_SELECT_NEXT_UNREAD) {
			list_menu_select_next_unread(&feeds_menu);
		} else if (cmd == INPUT_SELECT_PREV_UNREAD) {
			list_menu_select_prev_unread(&feeds_menu);
		} else if (cmd == INPUT_SELECT_NEXT_PAGE) {
			list_menu_select_next_page(&feeds_menu);
		} else if (cmd == INPUT_SELECT_PREV_PAGE) {
			list_menu_select_prev_page(&feeds_menu);
		} else if (cmd == INPUT_SELECT_FIRST) {
			list_menu_select_first(&feeds_menu);
		} else if (cmd == INPUT_SELECT_LAST) {
			list_menu_select_last(&feeds_menu);
		} else if (cmd == INPUT_MARK_READ) {
			mark_selected_feed_read();
		} else if (cmd == INPUT_MARK_UNREAD) {
			mark_selected_feed_unread();
		} else if (cmd == INPUT_MARK_READ_ALL) {
			mark_all_feeds_read();
		} else if (cmd == INPUT_MARK_UNREAD_ALL) {
			mark_all_feeds_unread();
		} else if (cmd == INPUT_RELOAD) {
			reload_current_feed();
		} else if (cmd == INPUT_RELOAD_ALL) {
			reload_all_feeds();
		} else if (cmd == INPUT_ENTER) {
			cmd = enter_items_menu_loop(&feeds[feeds_menu.view_sel], 1, CFG_MENU_ITEM_ENTRY_FORMAT);
			if (cmd == INPUT_QUIT_SOFT) {
				redraw_menu_list(&feeds_menu);
			} else if (cmd == INPUT_QUIT_HARD) {
				break;
			}
		} else if (cmd == INPUT_EXPLORE_MENU) {
			cmd = enter_items_menu_loop(feeds, feeds_count, CFG_MENU_EXPLORE_ITEM_ENTRY_FORMAT);
			if (cmd == INPUT_QUIT_SOFT) {
				redraw_menu_list(&feeds_menu);
			} else if (cmd == INPUT_QUIT_HARD) {
				break;
			}
		} else if (cmd == INPUT_STATUS_HISTORY_MENU) {
			cmd = enter_status_pager_view_loop();
			if (cmd == INPUT_QUIT_SOFT) {
				redraw_menu_list(&feeds_menu);
			} else if (cmd == INPUT_QUIT_HARD) {
				break;
			}
		} else if (cmd == INPUT_RESIZE) {
			redraw_menu_list(&feeds_menu);
		} else if ((cmd == INPUT_QUIT_SOFT) || (cmd == INPUT_QUIT_HARD)) {
			break;
		}
	}

	return cmd;
}
