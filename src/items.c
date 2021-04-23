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
	char *item_path = item_data_path(items_path, itemwin->item->index);
	mvwprintw(itemwin->window, 0, 0 + 5 * config_number, is_item_read(item_path) ? " " : "N");
	free(item_path);
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
load_item_list(char *data_path)
{
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
items_menu(char *data_path)
{
	if (item_list == NULL) {
		int load_status = load_item_list(data_path);
		if (load_status != 0) {
			free_items();
			return load_status;
		}
		items_path = data_path;
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
		int contents_status = contents_menu(data_path, item_list[view_sel].item->index);
		if (contents_status == MENU_EXIT) {
			free_items();
			return MENU_EXIT;
		} else if (contents_status == MENU_CONTENT_ERROR) {
			do_clean = 0;
		}
	} else if (dest == MENU_FEEDS || dest == MENU_EXIT) {
		free_items();
		return dest;
	}

	return items_menu(data_path);
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

char *
item_data_path(char *feed_path, int64_t index)
{
	if (feed_path == NULL) return NULL;
	char *path = malloc(sizeof(char) * MAXPATH);
	if (path == NULL) return NULL;
	strcpy(path, feed_path);
	char dir_name[MAX_ITEM_INDEX_LEN];
	sprintf(dir_name, "%" PRId64 "", index);
	strcat(path, dir_name);
	strcat(path, "/");
	mkdir(path, 0777);
	return path;
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
is_item_read(char *item_path)
{
	if (item_path == NULL) return 1;
	char *item = malloc(sizeof(char) * MAXPATH);
	if (item == NULL) return 1;
	strcpy(item, item_path);
	strcat(item, ISNEW_FILE);
	FILE *f = fopen(item, "r");
	free(item);
	if (f != NULL) {
		fclose(f);
		return 0;
	}
	return 1;
}
