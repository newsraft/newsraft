#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

#define SELECT_CMD_PART_1 "SELECT rowid, title, unread FROM items WHERE "
#define SELECT_CMD_PART_1_LEN 45
#define SELECT_CMD_PART_3 " ORDER BY upddate DESC, pubdate DESC, rowid DESC"
#define SELECT_CMD_PART_3_LEN 48

static struct item_line *items;
static size_t items_count;

static size_t view_sel; // index of selected item
static size_t view_min; // index of first visible item
static size_t view_max; // index of last visible item

static void
free_items(void)
{
	if (items == NULL) return;
	for (size_t i = 0; i < items_count; ++i) {
		free_string(items[i].title);
	}
	free(items);
}

static int
load_items(const struct set_condition *sc)
{
	char *cmd = malloc(sizeof(char) * (SELECT_CMD_PART_1_LEN + sc->db_cmd->len + SELECT_CMD_PART_3_LEN + 1));
	if (cmd == NULL) {
		status_write("Failed to load items: not enough memory!");
		debug_write(DBG_FAIL, "Not enough memory for loading items!\n");
		return 1; // failure
	}
	strcpy(cmd, SELECT_CMD_PART_1);
	strcat(cmd, sc->db_cmd->ptr);
	strcat(cmd, SELECT_CMD_PART_3);
	debug_write(DBG_INFO, "Items SELECT statement: %s\n", cmd);
	int error = 0;
	view_sel = SIZE_MAX;
	sqlite3_stmt *res;
	if (sqlite3_prepare_v2(db, cmd, -1, &res, 0) == SQLITE_OK) {
		size_t item_index;
		char *text;
		struct item_line *temp; // need to check if realloc failed
		for (size_t i = 0; i < sc->urls_count; ++i) {
			sqlite3_bind_text(res, i + 1, sc->urls[i]->ptr, sc->urls[i]->len, NULL);
		}
		while (sqlite3_step(res) == SQLITE_ROW) {
			item_index = items_count++;
			temp = realloc(items, sizeof(struct item_line) * items_count);
			if (temp == NULL) {
				debug_write(DBG_FAIL, "Not enough memory for loading items (realloc returned NULL)!\n");
				--items_count;
				error = 1;
				break;
			}
			items = temp;
			items[item_index].title = NULL;
			if (view_sel == SIZE_MAX) {
				view_sel = item_index;
			}
			items[item_index].rowid = sqlite3_column_int(res, 0);
			if ((text = (char *)sqlite3_column_text(res, 1)) != NULL) {
				items[item_index].title = create_string(text, strlen(text));
			}
			items[item_index].is_unread = sqlite3_column_int(res, 2);
		}
	} else {
		status_write("There is some error with the tag expression!");
		debug_write(DBG_FAIL, "Failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
		error = 1;
	}

	sqlite3_finalize(res);
	free(cmd);

	if (error == 1) {
		return 1; // failure
	}

	if (view_sel == SIZE_MAX) {
		status_write("Items not found!");
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
	if (db_make_item_read(items[index].rowid)) {
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
	if (db_make_item_unread(items[index].rowid)) {
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
redraw_items_windows(void)
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
	set_input_handler(INPUT_RESIZE, &redraw_items_windows);
}

static sqlite3_stmt *
find_item_by_its_rowid_in_db(int rowid)
{
	sqlite3_stmt *res;
	if (sqlite3_prepare_v2(db, "SELECT * FROM items WHERE rowid = ? LIMIT 1", -1, &res, 0) != SQLITE_OK) {
		DEBUG_WRITE_DB_PREPARE_FAIL;
		return NULL; // failure
	}
	sqlite3_bind_int(res, 1, rowid);
	if (sqlite3_step(res) != SQLITE_ROW) {
		debug_write(DBG_WARN, "Could not find an item with the rowid %d!\n", rowid);
		sqlite3_finalize(res);
		return NULL; // failure
	}
	debug_write(DBG_INFO, "Item with the rowid %d is found.\n", rowid);
	return res; // success
}

static struct string *
get_contents_of_item(sqlite3_stmt *res)
{
	struct string *buf = create_empty_string();
	if (buf == NULL) {
		return NULL;
	}

	if (cat_item_meta_data_to_buf(buf, res) != 0) {
		free_string(buf);
		return NULL;
	}

	char *text = (char *)sqlite3_column_text(res, ITEM_COLUMN_CONTENT);
	if (text != NULL) {
		size_t text_len = strlen(text);
		if (text_len != 0) {
			struct string *plain_text = plainify_html(text, text_len);
			if (plain_text != NULL) {
				if (plain_text->len != 0) {
					cat_string_char(buf, '\n');
					cat_string_string(buf, plain_text);
				}
				free_string(plain_text);
			}
		}
	}
	return buf;
}

int
enter_items_menu_loop(const struct set_condition *sc)
{
	items = NULL;
	items_count = 0;

	if (items == NULL) {
		if (load_items(sc) != 0) {
			free_items();
			return INPUTS_COUNT;
		}
	}

	view_min = 0;
	view_max = list_menu_height - 1;

	redraw_items_windows();

	set_items_input_handlers();

	int destination;
	struct string *contents;
	sqlite3_stmt *res;
	while (1) {
		destination = handle_input();
		if (destination == INPUT_ENTER) {
			debug_write(DBG_INFO, "Trying to view an item with the rowid %d.\n", items[view_sel].rowid);

			res = find_item_by_its_rowid_in_db(items[view_sel].rowid);
			if (res == NULL) {
				continue;
			}

			contents = get_contents_of_item(res);
			sqlite3_finalize(res);
			if (contents == NULL) {
				continue;
			}

			destination = pager_view(contents);

			db_make_item_read(items[view_sel].rowid);
			free_string(contents);

			if (destination == INPUT_QUIT_SOFT) {
				set_items_input_handlers();
				items[view_sel].is_unread = 0;
				redraw_items_windows();
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
