#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"

#define SELECT_CMD_PART_1 "SELECT feed, title, url, guid, unread FROM items WHERE"
#define SELECT_CMD_PART_3 " ORDER BY upddate DESC, pubdate DESC, rowid ASC"

static struct item_line *items = NULL;
static size_t items_count = 0;
static size_t view_sel;
// [view_min; view_max] is index range of displayed items
static size_t view_min;
static size_t view_max;

static int
load_items(struct set_condition *st)
{
	char *cmd = malloc(sizeof(SELECT_CMD_PART_1) + st->db_cmd->len + sizeof(SELECT_CMD_PART_3) + 1);
	if (cmd == NULL) {
		status_write("[error] not enough memory");
		return 1;
	}
	strcpy(cmd, SELECT_CMD_PART_1);
	strcat(cmd, st->db_cmd->ptr);
	strcat(cmd, SELECT_CMD_PART_3);
	debug_write(DBG_INFO, "item SELECT statement command: %s\n", cmd);
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
			items[item_index].data = calloc(1, sizeof(struct item_entry));
			if (items[item_index].data == NULL) {
				debug_write(DBG_ERR, "memory allocation for item data failed\n");
				status_write("[error] not enough memory");
				error = 1;
				break;
			}
			if (view_sel == SIZE_MAX) view_sel = item_index;
			if ((text = (char *)sqlite3_column_text(res, 0)) != NULL) {
				items[item_index].feed_url = create_string(text, strlen(text));
			}
			if ((text = (char *)sqlite3_column_text(res, 1)) != NULL) {
				items[item_index].data->title = create_string(text, strlen(text));
			}
			if ((text = (char *)sqlite3_column_text(res, 2)) != NULL) {
				items[item_index].data->url = create_string(text, strlen(text));
			}
			if ((text = (char *)sqlite3_column_text(res, 3)) != NULL) {
				items[item_index].data->guid = create_string(text, strlen(text));
			}
			items[item_index].is_unread = sqlite3_column_int(res, 4);
		}
	} else {
		status_write("[error] invalid tag expression");
		debug_write(DBG_ERR, "failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
		error = 1;
	}
	sqlite3_finalize(res);
	free(cmd);
	if (error == 1) {
		return 1;
	}
	if (view_sel == SIZE_MAX) {
		status_write("[error] no items found");
		return 1; // no items found
	}
	return 0;
}

// return most sensible string for item title
static char *
item_image(struct item_entry *item_data) {
	if (item_data != NULL) {
		if (item_data->title != NULL) {
			return item_data->title->ptr;
		}
	}
	return "untitled";
}

static void
item_expose(size_t index)
{
	struct item_line *item = &items[index];
	werase(item->window);
	if (config_menu_show_number == true) {
		mvwprintw(item->window, 0, 0, "%3d", index + 1);
		mvwprintw(item->window, 0, 6, item->is_unread ? "N" : " ");
		mvwprintw(item->window, 0, 9, "%s", item_image(item->data));
	} else {
		mvwprintw(item->window, 0, 3, item->is_unread ? "N" : " ");
		mvwprintw(item->window, 0, 6, "%s", item_image(item->data));
	}
	mvwchgat(item->window, 0, 0, -1, (index == view_sel) ? A_REVERSE : A_NORMAL, 0, NULL);
	wrefresh(item->window);
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
free_items(void)
{
	if (items == NULL) return;
	for (size_t i = 0; i < items_count; ++i) {
		free_string(items[i].feed_url);
		if (items[i].data != NULL) {
			free_string(items[i].data->title);
			free_string(items[i].data->url);
			free_string(items[i].data->guid);
			free(items[i].data);
		}
	}
	items_count = 0;
	free(items);
	items = NULL;
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

	if (items[new_sel].data == NULL || new_sel == view_sel) {
		return;
	}

	if (new_sel > view_max) {
		view_min = new_sel - LINES + 2;
		view_max = new_sel;
		view_sel = new_sel;
		show_items();
	} else if (new_sel < view_min) {
		view_min = new_sel;
		view_max = new_sel + LINES - 2;
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
mark_item_unread(size_t index, bool value)
{
	struct item_line *item = &items[index];
	if (db_update_item_int(item->feed_url, item->data, "unread", value)) {
		item->is_unread = value;
		item_expose(index);
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
input_mark_item_read(void)
{
	mark_item_unread(view_sel, false);
}

static void
input_mark_item_unread(void)
{
	mark_item_unread(view_sel, true);
}

static void
redraw_items_by_resize(void)
{
	view_max = view_min + LINES - 2;
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
	set_input_handler(INPUT_MARK_READ, &input_mark_item_read);
	set_input_handler(INPUT_MARK_UNREAD, &input_mark_item_unread);
	set_input_handler(INPUT_RESIZE, &redraw_items_by_resize);
}

int
enter_items_menu_loop(struct set_condition *st)
{
	if (items == NULL) {
		if (load_items(st) != 0) {
			free_items();
			return INPUTS_COUNT;
		}
	}
	view_min = 0;
	view_max = LINES - 2;
	set_items_input_handlers();
	clear();
	refresh();
	show_items();

	int dest;
	while (1) {
		dest = handle_input();
		if (dest == INPUT_SOFT_QUIT || dest == INPUT_HARD_QUIT) {
			break;
		} else if (dest == INPUT_ENTER) {
			dest = enter_item_contents_menu_loop(&items[view_sel]);
			if (dest == INPUT_HARD_QUIT) {
				break;
			} else if (dest == INPUT_SOFT_QUIT) {
				items[view_sel].is_unread = false;
				set_items_input_handlers();
				clear();
				refresh();
				show_items();
			}
		}
	}
	free_items();
	return dest;
}
