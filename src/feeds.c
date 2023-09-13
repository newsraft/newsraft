#include "newsraft.h"

static struct feed_entry **feeds = NULL;
static size_t feeds_count = 0;

static struct format_arg feeds_fmt_args[] = {
	{L'n',  L'd',  {.i = 0   }},
	{L'u',  L'd',  {.i = 0   }},
	{L'l',  L's',  {.s = NULL}},
	{L't',  L's',  {.s = NULL}},
	{L'o',  L's',  {.s = NULL}},
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
	feeds_fmt_args[3].value.s = feeds[index]->name ? feeds[index]->name->ptr : "";
	feeds_fmt_args[4].value.s = feeds[index]->name ? feeds[index]->name->ptr : feeds[index]->link->ptr;
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
enter_feeds_menu_loop(struct feed_entry **new_feeds, size_t new_feeds_count)
{
	input_cmd_id cmd;
	feeds = new_feeds;
	feeds_count = new_feeds_count;
	if (get_cfg_bool(CFG_FEEDS_MENU_PARAMOUNT_EXPLORE) == true) {
		cmd = enter_items_menu_loop(feeds, feeds_count, true, NULL);
		if (cmd == INPUT_QUIT_SOFT || cmd == INPUT_QUIT_HARD) {
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
			if (cmd == INPUT_QUIT_HARD) break;
			enter_list_menu(FEEDS_MENU, 0, false);
		} else if (cmd == INPUT_APPLY_SEARCH_MODE_FILTER) {
			cmd = enter_items_menu_loop(feeds, feeds_count, true, search_mode_text_input);
			if (cmd == INPUT_QUIT_HARD) break;
			enter_list_menu(FEEDS_MENU, 0, false);
		} else if (cmd == INPUT_STATUS_HISTORY_MENU) {
			cmd = enter_status_pager_view_loop();
			if (cmd == INPUT_QUIT_HARD) break;
			enter_list_menu(FEEDS_MENU, 0, false);
		} else if ((cmd == INPUT_QUIT_SOFT) || (cmd == INPUT_QUIT_HARD)) {
			break;
		}
	}
	return cmd;
}
