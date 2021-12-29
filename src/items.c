#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

static struct item_line *items;
static size_t items_count;

static size_t view_sel; // index of selected item
static size_t view_min; // index of first visible item
static size_t view_max; // index of last visible item

static struct format_arg fmt_args[] = {
	{'n', 'd', {.i = 0}},
	{'u', 'c', {.c = '\0'}},
	{'t', 's', {.s = NULL}},
};

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
#define SELECT_CMD_PART_1 "SELECT rowid, title, unread FROM items WHERE "
#define SELECT_CMD_PART_1_LEN 45
#define SELECT_CMD_PART_3 " ORDER BY upddate ASC, pubdate ASC, rowid ASC"
#define SELECT_CMD_PART_3_LEN 45
	char *cmd = malloc(sizeof(char) * (SELECT_CMD_PART_1_LEN + sc->db_cmd->len + SELECT_CMD_PART_3_LEN + 1));
	if (cmd == NULL) {
		status_write("Failed to load items: not enough memory!");
		FAIL("Not enough memory for loading items!");
		return 1;
	}
	strcpy(cmd, SELECT_CMD_PART_1);
	strcat(cmd, sc->db_cmd->ptr);
	strcat(cmd, SELECT_CMD_PART_3);
	view_sel = SIZE_MAX;
	sqlite3_stmt *res;
	if (db_prepare(cmd, -1, &res, NULL) != SQLITE_OK) {
		status_write("There is some error with the tag expression!");
		free(cmd);
		return 1;
	}
	size_t item_index;
	char *text;
	struct item_line *temp; // need to check if realloc failed
	for (size_t i = 0; i < sc->urls_count; ++i) {
		sqlite3_bind_text(res, i + 1, sc->urls[i]->ptr, sc->urls[i]->len, NULL);
	}
	int error = 0;
	while (sqlite3_step(res) == SQLITE_ROW) {
		item_index = items_count++;
		temp = realloc(items, sizeof(struct item_line) * items_count);
		if (temp == NULL) {
			FAIL("Not enough memory for loading items (realloc returned NULL)!");
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
	fmt_args[0].value.i = index + 1;
	fmt_args[1].value.c = items[index].is_unread == true ? 'N' : ' ';
	fmt_args[2].value.s = items[index].title->ptr;
	wprintw(items[index].window, "%s", do_format(config_menu_item_entry_format, fmt_args, LENGTH(fmt_args)));
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
	if (db_mark_item_read(items[index].rowid) != 0) {
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
	if (db_mark_item_unread(items[index].rowid) != 0) {
		return; // failure
	}
	items[index].is_unread = 1;
	if ((index >= view_min) && (index <= view_max)) {
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

static void
redraw_items_windows(void)
{
	clear();
	refresh();
	view_max = view_min + (list_menu_height - 1);
	if (view_max < view_sel) {
		view_max = view_sel;
		view_min = view_max - (list_menu_height - 1);
	}
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

static inline int
enter_item_pager_loop(int rowid)
{
	INFO("Trying to view an item with the rowid %d.", rowid);
	struct content_list *list = create_content_list_for_item(rowid);
	if (list == NULL) {
		FAIL("Can not create content list for item!");
		return INPUTS_COUNT;
	}
	int destination = pager_view(list);
	free_content_list(list);
	return destination;
}

int
enter_items_menu_loop(const struct set_condition *sc)
{
	items = NULL;
	items_count = 0;

	INFO("Loading items...");
	if (load_items(sc) != 0) {
		FAIL("Failed to load items!");
		free_items();
		return INPUTS_COUNT;
	}

	view_min = 0;
	view_max = list_menu_height - 1;

	redraw_items_windows();

	set_items_input_handlers();

	int destination;
	while (1) {
		destination = handle_input();
		if (destination == INPUT_ENTER) {

			destination = enter_item_pager_loop(items[view_sel].rowid);
			if (destination == INPUT_QUIT_SOFT) {
				items[view_sel].is_unread = 0;
				db_mark_item_read(items[view_sel].rowid);
				set_items_input_handlers();
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
