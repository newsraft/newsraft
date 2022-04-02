#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

static struct item_line *items;
static size_t items_count;

static size_t view_sel; // index of selected item
static size_t view_min; // index of first visible item
static size_t view_max; // index of last visible item

static struct format_arg fmt_args[] = {
	{L'n', L"d", {.i = 0}},
	{L'u', L"c", {.c = '\0'}},
	{L't', L"s", {.s = NULL}},
};

static void
free_items(void)
{
	if (items == NULL) {
		return;
	}
	for (size_t i = 0; i < items_count; ++i) {
		free_string(items[i].title);
	}
	free(items);
}

// On success returns true.
// On failure returns false.
static bool
load_items(const struct string *url)
{
	sqlite3_stmt *res;
	if (db_prepare("SELECT rowid, unread, title FROM items WHERE feed_url=? ORDER BY upddate DESC, pubdate DESC, rowid DESC;", 105, &res, NULL) == false) {
		status_write("There is some error with the tag expression!");
		return false;
	}
	sqlite3_bind_text(res, 1, url->ptr, url->len, NULL);
	view_sel = SIZE_MAX;
	size_t item_index;
	void *temp; // need to check if realloc failed
	bool error = false;
	while (sqlite3_step(res) == SQLITE_ROW) {
		item_index = items_count++;
		temp = realloc(items, sizeof(struct item_line) * items_count);
		if (temp == NULL) {
			FAIL("Not enough memory for loading items!");
			--items_count;
			error = true;
			break;
		}
		items = temp;
		items[item_index].rowid = sqlite3_column_int(res, 0);
		items[item_index].is_unread = sqlite3_column_int(res, 1);
		items[item_index].title = db_get_plain_text_from_column(res, 2);
		if (items[item_index].title == NULL) {
			error = true;
			break;
		}
		if (view_sel == SIZE_MAX) {
			view_sel = item_index;
		}
	}

	sqlite3_finalize(res);

	if (error == true) {
		return false;
	}

	if ((view_sel == SIZE_MAX) || (items_count == 0)) {
		status_write("Items not found!");
		return false;
	}

	return true;
}

static void
item_expose(size_t index)
{
	werase(items[index].window);
	fmt_args[0].value.i = index + 1;
	fmt_args[1].value.c = items[index].is_unread == true ? 'N' : ' ';
	fmt_args[2].value.s = items[index].title->ptr;
	mvwaddnwstr(items[index].window, 0, 0, do_format(cfg.menu_item_entry_format, fmt_args, COUNTOF(fmt_args)), list_menu_width);
	mvwchgat(items[index].window, 0, 0, -1, (index == view_sel) ? A_REVERSE : A_NORMAL, 0, NULL);
	wrefresh(items[index].window);
}

static void
show_items(void)
{
	for (size_t i = view_min, j = 0; i < items_count && i <= view_max; ++i, ++j) {
		items[i].window = get_list_entry_by_index(j);
		item_expose(i);
	}
}

static void
view_select(size_t i)
{
	size_t new_sel = i;

	if (new_sel >= items_count) {
		// Don't check if items_count is zero because program
		// won't even get here when not a single item loaded.
		new_sel = items_count - 1;
	}

	if (new_sel == view_sel) {
		return;
	}

	if (new_sel > view_max) {
		view_min = new_sel - (list_menu_height - 1);
		view_max = new_sel;
		view_sel = new_sel;
		show_items();
	} else if (new_sel < view_min) {
		view_min = new_sel;
		view_max = new_sel + (list_menu_height - 1);
		view_sel = new_sel;
		show_items();
	} else {
		mvwchgat(items[view_sel].window, 0, 0, -1, A_NORMAL, 0, NULL);
		wrefresh(items[view_sel].window);
		view_sel = new_sel;
		mvwchgat(items[view_sel].window, 0, 0, -1, A_REVERSE, 0, NULL);
		wrefresh(items[view_sel].window);
	}
}

static void
mark_item_read(size_t index)
{
	if (items[index].is_unread == 0) {
		return; // success, item is already read
	}
	if (db_mark_item_read(items[index].rowid) == false) {
		return; // failure
	}
	items[index].is_unread = 0;
	if ((index >= view_min) && (index <= view_max)) {
		item_expose(index);
	}
}

static void
mark_item_unread(size_t index)
{
	if (items[index].is_unread == 1) {
		return; // success, item is already unread
	}
	if (db_mark_item_unread(items[index].rowid) == false) {
		return; // failure
	}
	items[index].is_unread = 1;
	if ((index >= view_min) && (index <= view_max)) {
		item_expose(index);
	}
}

static void
mark_all_items_read(void)
{
	for (size_t i = 0; i < items_count; ++i) {
		mark_item_read(i);
	}
}

static void
mark_all_items_unread(void)
{
	for (size_t i = 0; i < items_count; ++i) {
		mark_item_unread(i);
	}
}

static void
redraw_items_windows(void)
{
	clear();
	refresh();
	status_update();
	view_max = view_min + (list_menu_height - 1);
	if (view_max < view_sel) {
		view_max = view_sel;
		view_min = view_max - (list_menu_height - 1);
	}
	show_items();
}

static inline int
enter_item_pager_loop(int rowid)
{
	INFO("Trying to view an item with rowid %d...", rowid);
	sqlite3_stmt *res = db_find_item_by_rowid(rowid);
	if (res == NULL) {
		return INPUTS_COUNT;
	}
	struct link_list links = {NULL, 0, 0};
	if (populate_link_list_with_links_of_item(&links, res) == false) {
		free_trim_link_list(&links);
		sqlite3_finalize(res);
		return INPUTS_COUNT;
	}
	struct render_block *first_block = NULL;
	if (join_render_blocks_of_item_data(&first_block, res) == false) {
		goto error;
	}
	if (prepare_to_render_data(first_block, &links) == false) {
		goto error;
	}
	if (complete_urls_of_links(&links, res) == false) {
		goto error;
	}
	if (cfg.append_links == true) {
		if (join_links_render_block(&first_block, &links) == false) {
			goto error;
		}
	}
	int destination = pager_view(first_block);
	free_render_blocks(first_block);
	free_trim_link_list(&links);
	sqlite3_finalize(res);
	return destination;
error:
	free_render_blocks(first_block);
	free_trim_link_list(&links);
	sqlite3_finalize(res);
	return INPUTS_COUNT;
}

input_cmd_id
enter_items_menu_loop(const struct string *url)
{
	items = NULL;
	items_count = 0;

	INFO("Loading items...");
	if (load_items(url) == false) {
		WARN("Failed to load items!");
		free_items();
		return INPUTS_COUNT;
	}

	view_min = 0;
	view_max = list_menu_height - 1;

	status_clean();
	redraw_items_windows();

	input_cmd_id cmd;
	while (true) {
		cmd = get_input_command();
		if (cmd == INPUT_SELECT_NEXT) {
			view_select(view_sel + 1);
		} else if (cmd == INPUT_SELECT_PREV) {
			view_select(view_sel == 0 ? (0) : (view_sel - 1));
		} else if (cmd == INPUT_SELECT_FIRST) {
			view_select(0);
		} else if (cmd == INPUT_SELECT_LAST) {
			// Don't check if items_count is equal to zero,
			// because we won't even get here if none items loaded.
			view_select(items_count - 1);
		} else if (cmd == INPUT_MARK_READ) {
			mark_item_read(view_sel);
		} else if (cmd == INPUT_MARK_UNREAD) {
			mark_item_unread(view_sel);
		} else if (cmd == INPUT_MARK_READ_ALL) {
			mark_all_items_read();
		} else if (cmd == INPUT_MARK_UNREAD_ALL) {
			mark_all_items_unread();
		} else if (cmd == INPUT_ENTER) {
			cmd = enter_item_pager_loop(items[view_sel].rowid);
			if (cmd == INPUT_QUIT_SOFT) {
				items[view_sel].is_unread = 0;
				db_mark_item_read(items[view_sel].rowid);
				redraw_items_windows();
			} else if (cmd == INPUT_QUIT_HARD) {
				break;
			}
		} else if (cmd == INPUT_RESIZE) {
			redraw_items_windows();
		} else if ((cmd == INPUT_QUIT_SOFT) || (cmd == INPUT_QUIT_HARD)) {
			break;
		}
	}

	free_items();

	return cmd;
}
