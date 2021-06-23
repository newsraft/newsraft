#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"
#include "config.h"

#define SELECT_CMD_PART_1 "SELECT feed, title, url, guid, marked, unread FROM items WHERE"
#define SELECT_CMD_PART_3 " ORDER BY pubdate DESC"

static struct item_line *item_list = NULL;
static size_t item_count = 0;
static size_t view_sel;
// [view_min; view_max] is index range of displayed items
static size_t view_min;
static size_t view_max;

static int
load_item_list(struct set_statement *st)
{
	size_t item_index;
	int error = 0;
	view_sel = SIZE_MAX;
	sqlite3_stmt *res;
	char *cmd, *text;
	cmd = malloc(sizeof(SELECT_CMD_PART_1) + st->db_cmd->len + sizeof(SELECT_CMD_PART_3) + 1);
	strcpy(cmd, SELECT_CMD_PART_1);
	strcat(cmd, st->db_cmd->ptr);
	strcat(cmd, SELECT_CMD_PART_3);
	debug_write(DBG_INFO, "item SELECT statement command: %s\n", cmd);
	int rc = sqlite3_prepare_v2(db, cmd, -1, &res, 0);
	if (rc == SQLITE_OK) {
		for (size_t i = 0; i < st->urls_count; ++i) {
			sqlite3_bind_text(res, i + 1, st->urls[i]->ptr, st->urls[i]->len, NULL);
		}
		while (1) {
			rc = sqlite3_step(res);
			if (rc != SQLITE_ROW) break;
			item_index = item_count++;
			item_list = realloc(item_list, sizeof(struct item_line) * item_count);
			item_list[item_index].feed_url = NULL;
			item_list[item_index].data = calloc(1, sizeof(struct item_entry));
			if (item_list[item_index].data == NULL) {
				debug_write(DBG_ERR, "memory allocation for item data failed\n");
				error = 1;
				break;
			}
			if (view_sel == SIZE_MAX) view_sel = item_index;
			if ((text = (char *)sqlite3_column_text(res, 0)) != NULL) {
				make_string(&item_list[item_index].feed_url, text, strlen(text));
			}
			if ((text = (char *)sqlite3_column_text(res, 1)) != NULL) {
				make_string(&item_list[item_index].data->title, text, strlen(text));
			}
			if ((text = (char *)sqlite3_column_text(res, 2)) != NULL) {
				make_string(&item_list[item_index].data->url, text, strlen(text));
			}
			if ((text = (char *)sqlite3_column_text(res, 3)) != NULL) {
				make_string(&item_list[item_index].data->guid, text, strlen(text));
			}
			item_list[item_index].is_marked = sqlite3_column_int(res, 4);
			item_list[item_index].is_unread = sqlite3_column_int(res, 5);
		}
	} else {
		debug_write(DBG_ERR, "failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
		error = 1;
	}
	sqlite3_finalize(res);
	free(cmd);
	if (error == 1) {
		return MENU_ITEMS_ERROR;
	}
	if (view_sel == SIZE_MAX) {
		return MENU_ITEMS_EMPTY; // no items found
	}
	return MENU_ITEMS;
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
	struct item_line *item = &item_list[index];
	if (config_menu_show_number == true) {
		mvwprintw(item->window, 0, 0, "%3d", index + 1);
		mvwprintw(item->window, 0, 5, item->is_marked ? "M" : " ");
		mvwprintw(item->window, 0, 6, item->is_unread ? "N" : " ");
		mvwprintw(item->window, 0, 9, "%s", item_image(item->data));
	} else {
		mvwprintw(item->window, 0, 2, item->is_marked ? "M" : " ");
		mvwprintw(item->window, 0, 3, item->is_unread ? "N" : " ");
		mvwprintw(item->window, 0, 6, "%s", item_image(item->data));
	}
	mvwchgat(item->window, 0, 0, -1, (index == view_sel) ? A_REVERSE : A_NORMAL, 0, NULL);
	wrefresh(item->window);
}

static void
show_items(void)
{
	for (size_t i = view_min, j = 0; i < item_count && i <= view_max; ++i, ++j) {
		item_list[i].window = newwin(1, COLS, j, 0);
		item_expose(i);
	}
}

void
hide_items(void)
{
	for (size_t i = view_min; i < item_count && i <= view_max; ++i) {
		if (item_list[i].window != NULL) {
			delwin(item_list[i].window);
		}
	}
}

static void
free_items(void)
{
	if (item_list == NULL) return;
	for (size_t i = 0; i < item_count; ++i) {
		if (item_list[i].data != NULL) {
			if (item_list[i].feed_url != NULL) free_string(&item_list[i].feed_url);
			if (item_list[i].data->title != NULL) free_string(&item_list[i].data->title);
			if (item_list[i].data->url != NULL) free_string(&item_list[i].data->url);
			if (item_list[i].data->guid != NULL) free_string(&item_list[i].data->guid);
			free(item_list[i].data);
		}
	}
	item_count = 0;
	free(item_list);
	item_list = NULL;
}

static void
view_select(size_t i)
{
	size_t new_sel = i;

	// perform boundary check
	if (new_sel >= item_count) {
		if (item_count == 0) return;
		new_sel = item_count - 1;
	}

	if (item_list[new_sel].data != NULL && new_sel != view_sel) {
		if (new_sel > view_max) {
			hide_items();
			view_min = new_sel - LINES + 2;
			view_max = new_sel;
			view_sel = new_sel;
			show_items();
		} else if (new_sel < view_min) {
			hide_items();
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
}

static void
mark_item_marked(size_t index, bool value)
{
	struct item_line *item = &item_list[index];
	if (db_update_item_int(item->feed_url, item->data, "marked", value)) {
		item->is_marked = value;
		item_expose(index);
	}
}

static void
mark_item_unread(size_t index, bool value)
{
	struct item_line *item = &item_list[index];
	if (db_update_item_int(item->feed_url, item->data, "unread", value)) {
		item->is_unread = value;
		item_expose(index);
	}
}

static enum menu_dest
menu_items(void)
{
	int ch, q, i;
	char cmd[7];
	while (1) {
		ch = input_wgetch();
		if      (ch == 'j' || ch == KEY_DOWN)                            { view_select(view_sel + 1); }
		else if ((ch == 'k' || ch == KEY_UP) && (view_sel != 0))         { view_select(view_sel - 1); }
		else if (ch == config_key_mark_marked)                           { mark_item_marked(view_sel, true); }
		else if (ch == config_key_mark_unmarked)                         { mark_item_marked(view_sel, false); }
		else if (ch == config_key_mark_read)                             { mark_item_unread(view_sel, false); }
		else if (ch == config_key_mark_unread)                           { mark_item_unread(view_sel, true); }
		else if ((ch == 'G' || ch == KEY_END) && (item_count != 0))      { view_select(item_count - 1); }
		else if ((ch == 'g' && input_wgetch() == 'g') || ch == KEY_HOME) { view_select(0); }
		else if (isdigit(ch)) {
			q = 0;
			while (1) {
				cmd[q++] = ch;
				if (q > 6) break;
				cmd[q] = '\0';
				ch = input_wgetch();
				if (!isdigit(ch)) {
					i = atoi(cmd);
					if (ch == 'j') {
						view_select(view_sel + i);
					} else if ((ch == 'k') && (i <= view_sel)) {
						view_select(view_sel - i);
					} else if ((ch == 'G') && (i != 0)) {
						view_select(i - 1);
					}
					break;
				}
			}
		} 
		else if (ch == 'l' || ch == KEY_RIGHT || ch == '\n' || ch == KEY_ENTER) { return MENU_CONTENT; }
		else if (ch == 'h' || ch == KEY_LEFT || ch == config_key_soft_quit)     { return MENU_FEEDS; }
		else if (ch == config_key_hard_quit)                                    { return MENU_QUIT; }
	}
}

int
run_items_menu(struct set_statement *st)
{
	if (item_list == NULL) {
		int load_status = load_item_list(st);
		if (load_status != MENU_ITEMS) {
			free_string(&st->db_cmd);
			free(st->urls);
			free(st);
			free_items();
			return load_status;
		}
	}
	view_min = 0;
	view_max = LINES - 2;
	hide_sets();
	clear();
	refresh();
	show_items();

	int dest;
	while ((dest = menu_items()) != MENU_QUIT) {
		if (dest == MENU_CONTENT) {
			dest = contents_menu(&item_list[view_sel]);
			if (dest == MENU_QUIT) {
				break;
			} else if (dest == MENU_ITEMS) {
				item_list[view_sel].is_unread = false;
				clear();
				refresh();
				show_items();
			}
		} else if (dest == MENU_FEEDS || dest == MENU_QUIT) {
			hide_items();
			break;
		}
	}
	free_string(&st->db_cmd);
	free(st->urls);
	free(st);
	free_items();
	return dest;
}
