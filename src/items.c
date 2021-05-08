#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"
#include "config.h"

static struct string *feed_url = NULL;
static struct item_window *item_list = NULL;
static int item_count = 0;
static int view_sel = -1;
static int view_min;
static int view_max;
static int do_redraw = 1;

static int
load_item_list(void)
{
	int item_index;
	sqlite3_stmt *res;
	char cmd[] = "SELECT title, url, guid, marked, unread FROM items WHERE feed = ? ORDER BY pubdate DESC", *text;
	int rc = sqlite3_prepare_v2(db, cmd, -1, &res, 0);
	if (rc == SQLITE_OK) {
		sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
		while (1) {
			rc = sqlite3_step(res);
			if (rc != SQLITE_ROW) break;
			item_index = item_count++;
			item_list = realloc(item_list, sizeof(struct item_window) * item_count);
			item_list[item_index].item = calloc(1, sizeof(struct item_entry));
			if (item_list[item_index].item == NULL) {
				fprintf(stderr, "memory allocation for item entry failed\n"); break;
			}
			if (view_sel == -1) view_sel = item_index;
			item_list[item_index].index = item_index;
			if ((text = (char *)sqlite3_column_text(res, 0)) != NULL) {
				make_string(&item_list[item_index].item->title, text, strlen(text));
			}
			if ((text = (char *)sqlite3_column_text(res, 1)) != NULL) {
				make_string(&item_list[item_index].item->url, text, strlen(text));
			}
			if ((text = (char *)sqlite3_column_text(res, 2)) != NULL) {
				make_string(&item_list[item_index].item->guid, text, strlen(text));
			}
			item_list[item_index].is_marked = sqlite3_column_int(res, 3);
			item_list[item_index].is_unread = sqlite3_column_int(res, 4);
		}
	} else {
		fprintf(stderr, "failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(res);
	if (view_sel == -1) return MENU_ITEMS_EMPTY; // no items found
	return MENU_ITEMS;
}

// return most sensible string for item title
static char *
item_image(struct item_entry *item) {
	if (item != NULL) {
		if (item->title != NULL) {
			return item->title->ptr;
		}
	}
	return "untitled";
}

static void
item_expose(struct item_window *itemwin, bool highlight)
{
	if (config_menu_show_number == true) {
		mvwprintw(itemwin->window, 0, 0, "%3d", itemwin->index + 1);
		mvwprintw(itemwin->window, 0, 5, itemwin->is_marked ? "M" : " ");
		mvwprintw(itemwin->window, 0, 6, itemwin->is_unread ? "N" : " ");
		if (highlight) wattron(itemwin->window, A_REVERSE);
		mvwprintw(itemwin->window, 0, 9, "%s", item_image(itemwin->item));
	} else {
		mvwprintw(itemwin->window, 0, 2, itemwin->is_marked ? "M" : " ");
		mvwprintw(itemwin->window, 0, 3, itemwin->is_unread ? "N" : " ");
		if (highlight) wattron(itemwin->window, A_REVERSE);
		mvwprintw(itemwin->window, 0, 6, "%s", item_image(itemwin->item));
	}
	if (highlight) wattroff(itemwin->window, A_REVERSE);
	wrefresh(itemwin->window);
}

static void
show_items(void)
{
	for (int i = view_min, j = 0; i < item_count && i < view_max; ++i, ++j) {
		item_list[i].window = newwin(1, COLS, j, 0);
		item_expose(&item_list[i], (i == view_sel));
	}
}

void
hide_items(void)
{
	for (int i = view_min; i < item_count && i < view_max; ++i) {
		if (item_list[i].window != NULL) {
			delwin(item_list[i].window);
		}
	}
}

static void
free_items(void)
{
	if (item_list == NULL) return;
	for (int i = 0; i < item_count; ++i) {
		if (item_list[i].item != NULL) {
			if (item_list[i].item->title != NULL) free_string(&item_list[i].item->title);
			if (item_list[i].item->url != NULL) free_string(&item_list[i].item->url);
			if (item_list[i].item->guid != NULL) free_string(&item_list[i].item->guid);
			free(item_list[i].item);
		}
	}
	feed_url = NULL;
	view_sel = -1;
	item_count = 0;
	free(item_list);
	item_list = NULL;
}

static void
view_select(int i)
{
	int new_sel = i;
	if (new_sel < 0) {
		new_sel = 0;
	} else if (new_sel >= item_count) {
		new_sel = item_count - 1;
		if (new_sel < 0) return;
	}

	if (new_sel != view_sel) {
		if (new_sel >= view_max) {
			hide_items();
			view_min += new_sel - view_sel;
			view_max += new_sel - view_sel;
			if (view_max > item_count) {
				view_max = item_count;
				view_min = view_max - LINES + 1;
			}
			view_sel = new_sel;
			show_items();
		} else if (new_sel < view_min) {
			hide_items();
			view_min -= view_sel - new_sel;
			view_max -= view_sel - new_sel;
			if (view_min < 0) {
				view_min = 0;
				view_max = LINES - 1;
			}
			view_sel = new_sel;
			show_items();
		} else {
			item_expose(&item_list[view_sel], 0);
			view_sel = new_sel;
			item_expose(&item_list[view_sel], 1);
		}
	}
}

static void
mark_item_marked(struct item_window *itemwin, bool value)
{
	if (db_change_item_int(feed_url, itemwin->item, ITEM_MARKED_STATE, value)) {
		itemwin->is_marked = value;
		item_expose(itemwin, 1);
	}
}

static void
mark_item_unread(struct item_window *itemwin, bool value)
{
	if (db_change_item_int(feed_url, itemwin->item, ITEM_UNREAD_STATE, value)) {
		itemwin->is_unread = value;
		item_expose(itemwin, 1);
	}
}

static enum menu_dest
menu_items(void)
{
	int ch, q;
	char cmd[7];
	while (1) {
		ch = input_wgetch();
		if      (ch == 'j' || ch == KEY_DOWN)                                   { view_select(view_sel + 1); }
		else if (ch == 'k' || ch == KEY_UP)                                     { view_select(view_sel - 1); }
		else if (ch == 'l' || ch == KEY_RIGHT || ch == '\n' || ch == KEY_ENTER) { return MENU_CONTENT; }
		else if (ch == 'h' || ch == KEY_LEFT)                                   { return MENU_FEEDS; }
		else if (ch == config_key_mark_marked)                                  { mark_item_marked(&item_list[view_sel], true); }
		else if (ch == config_key_mark_unmarked)                                { mark_item_marked(&item_list[view_sel], false); }
		else if (ch == config_key_mark_read)                                    { mark_item_unread(&item_list[view_sel], false); }
		else if (ch == config_key_mark_unread)                                  { mark_item_unread(&item_list[view_sel], true); }
		else if (ch == config_key_quit)                                         { return MENU_QUIT; }
		else if (ch == 'G')                                                     { view_select(item_count - 1); }
		else if (ch == 'g' && input_wgetch() == 'g')                            { view_select(0); }
		else if (isdigit(ch)) {
			q = 0;
			while (1) {
				cmd[q++] = ch;
				if (q > 6) break;
				cmd[q] = '\0';
				ch = input_wgetch();
				if (!isdigit(ch)) {
					if (ch == 'j') {
						view_select(view_sel + atoi(cmd));
					} else if (ch == 'k') {
						view_select(view_sel - atoi(cmd));
					} else if (ch == 'G') {
						view_select(atoi(cmd) - 1);
					}
					break;
				}
			}
		} 
	}
}

int
items_menu(struct string *items_feed_url)
{
	if (item_list == NULL) {
		feed_url = items_feed_url;
		int load_status = load_item_list();
		if (load_status != MENU_ITEMS) {
			free_items();
			return load_status;
		}
		view_min = 0;
		view_max = LINES - 1;
		hide_feeds();
	}

	if (do_redraw == 1) {
		clear();
		refresh();
		show_items();
	} else {
		do_redraw = 1;
	}

	int dest = menu_items();

	if (dest == MENU_CONTENT) {
		int contents_status = contents_menu(feed_url, item_list[view_sel].item);
		if (contents_status == MENU_QUIT) {
			free_items();
			return MENU_QUIT;
		} else if (contents_status == MENU_CONTENT_ERROR) {
			do_redraw = 0;
		} else if (contents_status == MENU_ITEMS) {
			item_list[view_sel].is_unread = false;
		}
	} else if (dest == MENU_FEEDS || dest == MENU_QUIT) {
		hide_items();
		free_items();
		return dest;
	}

	return items_menu(feed_url);
}
