#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"
#include "config.h"

static char *items_path = NULL;
static struct item_window *item_list = NULL;
static int item_count = 0;
static int view_sel = -1;
static int view_min;
static int view_max;
static int do_clean = 1;

static enum menu_dest menu_items(void);

// return most sensible string for item title
static char *
item_image(struct item_entry *item) {
	if (item != NULL) {
		if (item->name != NULL) {
			return item->name;
		}
	}
	return "untitled";
}

static void
item_expose(struct item_window *itemwin, bool highlight)
{
	if (config_number == 1) {
		mvwprintw(itemwin->window, 0, 0, "%3d", itemwin->index + 1);
	}
	mvwprintw(itemwin->window, 0, 0 + 5 * config_number, itemwin->item->unread ? "N" : " ");
	if (highlight) wattron(itemwin->window, A_REVERSE);
	mvwprintw(itemwin->window, 0, 3 + 5 * config_number, "%s", item_image(itemwin->item));
	if (highlight) wattroff(itemwin->window, A_REVERSE);
	wrefresh(itemwin->window);
}

static void
free_items(void)
{
	if (item_list == NULL) return;
	for (int i = 0; i < item_count; ++i) {
		if (item_list[i].item != NULL) {
			if (item_list[i].item->name != NULL) free(item_list[i].item->name);
			if (item_list[i].item->url != NULL) free(item_list[i].item->url);
			if (item_list[i].item->guid != NULL) free(item_list[i].item->guid);
			free(item_list[i].item);
		}
	}
	items_path = NULL;
	view_sel = -1;
	item_count = 0;
	free(item_list);
	item_list = NULL;
}

static int
load_item_list(char *feed_url)
{
	int item_index, rc, unread;
	sqlite3_stmt *res;
	char cmd[] = "SELECT name, link, guid, unread FROM items WHERE feed = ? ORDER BY pubdate DESC", *text;
	rc = sqlite3_prepare_v2(db, cmd, -1, &res, 0);
	if (rc == SQLITE_OK) {
		sqlite3_bind_text(res, 1, feed_url, strlen(feed_url), NULL);
		while (1) {
			rc = sqlite3_step(res);
			if (rc != SQLITE_ROW) break;
			item_index = item_count++;
			item_list = realloc(item_list, sizeof(struct item_window) * item_count);
			item_list[item_index].item = calloc(1, sizeof(struct item_entry));
			if (view_sel == -1) view_sel = item_index;
			item_list[item_index].index = item_index;
			if (item_list[item_index].item == NULL) {
				fprintf(stderr, "memory allocation for item entry failed\n"); break;
			}
			if ((text = (char *)sqlite3_column_text(res, 0)) != NULL) {
				item_list[item_index].item->name = malloc(sizeof(char) * (strlen(text) + 1));
				strcpy(item_list[item_index].item->name, text);
			}
			if ((text = (char *)sqlite3_column_text(res, 1)) != NULL) {
				item_list[item_index].item->url = malloc(sizeof(char) * (strlen(text) + 1));
				strcpy(item_list[item_index].item->url, text);
			}
			if ((text = (char *)sqlite3_column_text(res, 2)) != NULL) {
				item_list[item_index].item->guid = malloc(sizeof(char) * (strlen(text) + 1));
				strcpy(item_list[item_index].item->guid, text);
			}
			unread = sqlite3_column_int(res, 3);
			item_list[item_index].item->unread = unread;
		}
	} else {
		fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(res);
	if (view_sel == -1) {
		return MENU_ITEMS_EMPTY; // no items found
	}
	return 0;
}

static void
show_items(void)
{
	for (int i = view_min, j = 0; i < item_count && i < view_max; ++i, ++j) {
		item_list[i].window = newwin(1, COLS - config_left_offset, j + config_top_offset, config_left_offset);
		item_expose(&item_list[i], (i == view_sel));
	}
}

static void
hide_items(void)
{
	for (int i = view_min; i < item_count && i < view_max; ++i) {
		if (item_list[i].window != NULL) {
			delwin(item_list[i].window);
		}
	}
}

int
items_menu(char *feed_url)
{
	if (item_list == NULL) {
		int load_status = load_item_list(feed_url);
		if (load_status != 0) {
			free_items();
			return load_status;
		}
		/*items_path = data_path;*/
		view_min = 0;
		view_max = LINES - 1;
	}

	if (do_clean == 1) {
		clear();
		refresh();
	} else {
		do_clean = 1;
	}

	show_items();
	int dest = menu_items();
	hide_items();
	if (dest == MENU_CONTENT) {
		int contents_status = contents_menu(feed_url, item_list[view_sel].item);
		if (contents_status == MENU_EXIT) {
			free_items();
			return MENU_EXIT;
		} else if (contents_status == MENU_CONTENT_ERROR) {
			do_clean = 0;
		} else {
			item_list[view_sel].item->unread = false;
		}
	} else if (dest == MENU_FEEDS || dest == MENU_EXIT) {
		free_items();
		return dest;
	}

	return items_menu(feed_url);
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

static enum menu_dest
menu_items(void)
{
	int ch, q;
	char cmd[7];
	while (1) {
		ch = wgetch(input_win);
		wrefresh(input_win);
		if      (ch == 'j' || ch == KEY_DOWN)                                   { view_select(view_sel + 1); }
		else if (ch == 'k' || ch == KEY_UP)                                     { view_select(view_sel - 1); }
		else if (ch == 'l' || ch == KEY_RIGHT || ch == '\n' || ch == KEY_ENTER) { return MENU_CONTENT; }
		else if (ch == 'h' || ch == KEY_LEFT)                                   { return MENU_FEEDS; }
		else if (ch == config_key_exit)                                         { return MENU_EXIT; }
		else if (ch == 'G')                                                     { view_select(item_count - 1); }
		else if (ch == 'g' && wgetch(input_win) == 'g')                         { view_select(0); }
		else if (isdigit(ch)) {
			q = 0;
			while (1) {
				cmd[q++] = ch;
				if (q > 6) break;
				cmd[q] = '\0';
				ch = wgetch(input_win);
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

struct buf *
read_item_element(char *item_path, char *element)
{
	char *path = malloc(sizeof(char) * MAXPATH);
	if (path == NULL) return NULL;
	strcpy(path, item_path);
	strcat(path, element);
	FILE *f = fopen(path, "r");
	free(path);
	if (f == NULL) return NULL;
	struct buf *str = malloc(sizeof(struct buf));
	if (str == NULL) return NULL;
	str->len = 64;
	str->ptr = malloc(sizeof(char) * str->len);
	int count = 0;
	char c;
	while ((c = fgetc(f)) != EOF) {
		str->ptr[count++] = c;
		if (count == str->len) {
			str->len *= 2;
			str->ptr = realloc(str->ptr, sizeof(char) * str->len);
		}
	}
	str->ptr[count++] = '\0';
	str->len = count;
	str->ptr = realloc(str->ptr, sizeof(char) * str->len);
	fclose(f);
	if (count == 0) {
		free_string_ptr(str);
		return NULL;
	}
	return str;
}

int
try_item_bucket(struct item_bucket *bucket, char *feed_url)
{
	if (bucket == NULL || feed_url == NULL) return 0;
	sqlite3_stmt *res;
	char cmd[] = "SELECT * FROM items WHERE feed = ? AND guid = ? AND link = ?";
	int rc = sqlite3_prepare_v2(db, cmd, -1, &res, 0);
	if (rc == SQLITE_OK) {
		sqlite3_bind_text(res, 1, feed_url, strlen(feed_url), NULL);
		sqlite3_bind_text(res, 2, bucket->uid.ptr, bucket->uid.len - 1, NULL);
		sqlite3_bind_text(res, 3, bucket->link.ptr, bucket->link.len - 1, NULL);
		// if nothing found (item is unique), insert item into table
		if (sqlite3_step(res) == SQLITE_DONE) db_insert_item(bucket, feed_url);
	} else {
		fprintf(stderr, "failed to execute statement: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(res);
	return 1;
}
