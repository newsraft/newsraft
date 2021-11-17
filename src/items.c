#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"

#define SELECT_CMD_PART_1 "SELECT rowid, feed, title, unread FROM items WHERE"
#define SELECT_CMD_PART_3 " ORDER BY upddate DESC, pubdate DESC, rowid ASC"

static struct item_line *items = NULL;
static size_t items_count = 0;

static size_t view_sel; // index of selected item
static size_t view_min; // index of first visible item
static size_t view_max; // index of last visible item

static void
free_items(void)
{
	if (items == NULL) return;
	for (size_t i = 0; i < items_count; ++i) {
		free_string(items[i].feed_url);
		free_string(items[i].title);
	}
	free(items);
	items = NULL;
	items_count = 0;
}

static int
load_items(const struct set_condition *st)
{
	char *cmd = malloc(sizeof(SELECT_CMD_PART_1) + st->db_cmd->len + sizeof(SELECT_CMD_PART_3) + 1);
	if (cmd == NULL) {
		status_write("[error] not enough memory");
		return 1; // failure
	}
	strcpy(cmd, SELECT_CMD_PART_1);
	strcat(cmd, st->db_cmd->ptr);
	strcat(cmd, SELECT_CMD_PART_3);
	debug_write(DBG_INFO, "Item SELECT statement command: %s\n", cmd);
	int error = 0;
	view_sel = SIZE_MAX;
	sqlite3_stmt *res;
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		size_t item_index;
		char *text;
		for (size_t i = 0; i < st->urls_count; ++i) {
			sqlite3_bind_text(res, i + 1, st->urls[i]->ptr, st->urls[i]->len, NULL);
		}
		while (sqlite3_step(res) == SQLITE_ROW) {
			item_index = items_count++;
			items = realloc(items, sizeof(struct item_line) * items_count);
			items[item_index].feed_url = NULL;
			items[item_index].title = NULL;
			if (view_sel == SIZE_MAX) {
				view_sel = item_index;
			}
			items[item_index].rowid = sqlite3_column_int(res, 0);
			if ((text = (char *)sqlite3_column_text(res, 1)) != NULL) {
				items[item_index].feed_url = create_string(text, strlen(text));
			}
			if ((text = (char *)sqlite3_column_text(res, 2)) != NULL) {
				items[item_index].title = create_string(text, strlen(text));
			}
			items[item_index].is_unread = sqlite3_column_int(res, 3);
		}
	} else {
		status_write("[error] invalid tag expression");
		debug_write(DBG_FAIL, "Failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
		error = 1;
	}

	sqlite3_finalize(res);
	free(cmd);

	if (error == 1) {
		return 1; // failure
	}

	if (view_sel == SIZE_MAX) {
		status_write("[error] no items found");
		return 1; // failure, no items found
	}

	return 0; // success
}

static void
item_expose(size_t index)
{
	werase(items[index].window);
	print_item_format(index, &items[index]);
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

	// perform boundary check
	if (new_sel >= items_count) {
		if (items_count == 0) {
			return;
		}
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
		size_t old_sel = view_sel;
		view_sel = new_sel;
		item_expose(old_sel);
		item_expose(view_sel);
	}
}

static void
mark_item_read(size_t index)
{
	if (items[index].is_unread == 0) {
		return;
	}
	if (db_update_item_int(items[index].rowid, "unread", 0)) {
		items[index].is_unread = 0;
		if (index >= view_min && index <= view_max) {
			item_expose(index);
		}
	}
}

static void
mark_item_unread(size_t index)
{
	if (items[index].is_unread == 1) {
		return;
	}
	if (db_update_item_int(items[index].rowid, "unread", 1)) {
		items[index].is_unread = 1;
		if (index >= view_min && index <= view_max) {
			item_expose(index);
		}
	}
}

static void
view_select_next(void)
{
	view_select(view_sel + 1);
}

static void
view_select_prev(void)
{
	view_select(view_sel == 0 ? (0) : (view_sel - 1));
}

static void
view_select_first(void)
{
	view_select(0);
}

static void
view_select_last(void)
{
	view_select(items_count == 0 ? (0) : (items_count - 1));
}

static void
mark_selected_item_read(void)
{
	mark_item_read(view_sel);
}

static void
mark_selected_item_unread(void)
{
	mark_item_unread(view_sel);
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

void
resize_items_global_action(void)
{
	view_min = view_sel;
	view_max = view_min + (list_menu_height - 1);
}

static void
resize_items_local_action(void)
{
	clear();
	refresh();
	show_items();
}

static void
set_items_input_handlers(void)
{
	reset_input_handlers();
	set_input_handler(INPUT_SELECT_NEXT, &view_select_next);
	set_input_handler(INPUT_SELECT_PREV, &view_select_prev);
	set_input_handler(INPUT_SELECT_FIRST, &view_select_first);
	set_input_handler(INPUT_SELECT_LAST, &view_select_last);
	set_input_handler(INPUT_MARK_READ, &mark_selected_item_read);
	set_input_handler(INPUT_MARK_READ_ALL, &mark_all_items_read);
	set_input_handler(INPUT_MARK_UNREAD, &mark_selected_item_unread);
	set_input_handler(INPUT_MARK_UNREAD_ALL, &mark_all_items_unread);
	set_input_handler(INPUT_RESIZE, &resize_items_local_action);
}

int
enter_items_menu_loop(const struct set_condition *st)
{
	if (items == NULL) {
		if (load_items(st) != 0) {
			free_items();
			return INPUTS_COUNT;
		}
	}

	view_min = 0;
	view_max = list_menu_height - 1;

	clear();
	refresh();
	show_items();

	set_items_input_handlers();

	int destination;
	while (1) {
		destination = handle_input();
		if (destination == INPUT_ENTER) {
			destination = enter_item_contents_menu_loop(items[view_sel].rowid);
			if (destination == INPUT_QUIT_SOFT) {
				set_items_input_handlers();
				items[view_sel].is_unread = 0;
				clear();
				refresh();
				show_items();
			} else if (destination == INPUT_QUIT_HARD) {
				break;
			}
		} else if ((destination == INPUT_QUIT_SOFT) || (destination == INPUT_QUIT_HARD)) {
			break;
		}
	}

	free_items();

	return destination;
}
