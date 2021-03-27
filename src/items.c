#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "feedeater.h"
#include "config.h"

static struct item_window *item_list = NULL;
static int item_sel = -1;
static int item_count = 0;

static enum menu_dest menu_items(void);
static void free_item_list(void);

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

static int
load_item_list(char *data_path)
{
	int item_index;
	char c;
	int size = 32, count = 0;
	char item_num[8],
	     *value = malloc(sizeof(char) * size),
	     *path = malloc(sizeof(char) * MAXPATH);
	FILE *f;
	for (int i = 1; i <= MAX_ITEMS; ++i) {
		strcpy(path, data_path);
		sprintf(item_num, "%d/", i);
		strcat(path, item_num);
		strcat(path, "title");
		f = fopen(path, "r");
		if (f == NULL) continue;
		count = 0;
		while ((c = fgetc(f)) != EOF) {
			value[count++] = c;
			if (count == size) {
				size *= 2;
				value = realloc(value, sizeof(char) * size);
			}
		}
		value[count] = '\0';
		if (count != 0) {
			item_index = item_count++;
			item_list = realloc(item_list, sizeof(struct item_window) * item_count);
			if (item_list != NULL) {
				item_list[item_index].item = calloc(1, sizeof(struct item_entry));
				if (item_list[item_index].item != NULL) {
					item_list[item_index].item->name = malloc(sizeof(char) * (count + 1));
					if (item_list[item_index].item->name != NULL) {
						strcpy(item_list[item_index].item->name, value);
						if (item_sel == -1) item_sel = item_index;
					}
				}
			}
		}

		fclose(f);
	}

	free(value);
	free(path);

	if (item_sel == -1) {
		status_write("this feed is empty");
		return 0;
	}

	return 1;
}

void contents_menu(void) { status_write("viewing functionaltiy is under construction"); }

void
hide_items(void)
{
	for (int i = 0; i < item_count; ++i) {
		if (item_list[i].window != NULL) {
			delwin(item_list[i].window);
		}
	}
}

void
items_menu(char *url)
{
	if (item_list == NULL) {
		char *data_path = get_data_dir_path_for_url(url);
		if (data_path == NULL) {
			status_write("[error] could not access data directory for %s", url);
			return;
		}
		if (load_item_list(data_path) == 0) {
			free(data_path);
			status_write("[error] could not load data for %s", url);
			free_item_list();
			return;
		}
		free(data_path);
	}

	clear();
	refresh();
	for (int i = 0; i < item_count; ++i) {
		item_list[i].window = newwin(1, COLS, i, 0);
		if (i == item_sel) wattron(item_list[i].window, A_REVERSE);
		mvwprintw(item_list[i].window, 0, left_offset, "%s\n", item_image(item_list[i].item));
		if (i == item_sel) wattroff(item_list[i].window, A_REVERSE);
		wrefresh(item_list[i].window);
	}

	enum menu_dest dest = menu_items();
	hide_items();
	if (dest == MENU_CONTENTS) {
		contents_menu();
	} else if (dest == MENU_EXIT) {
		free_item_list();
		return;
	}

	items_menu(url);
}

static void
free_item_list(void)
{
	if (item_list == NULL) return;
	for (int i = 0; i < item_count; ++i) {
		if (item_list[i].item != NULL) {
			if (item_list[i].item->name != NULL) free(item_list[i].item->name);
			if (item_list[i].item->url != NULL) free(item_list[i].item->url);
			free(item_list[i].item);
		}
	}
	item_sel = -1;
	item_count = 0;
	free(item_list);
	item_list = NULL;
}

static void
item_select(int i)
{
	int new_sel = i;
	if (new_sel < 0) {
		new_sel = 0;
	} else if (new_sel >= item_count) {
		new_sel = item_count - 1;
		if (new_sel < 0) return;
	}
	if (new_sel != item_sel) {
		mvwprintw(item_list[item_sel].window, 0, left_offset, "%s\n", item_image(item_list[item_sel].item));
		wrefresh(item_list[item_sel].window);
		item_sel = new_sel;
		wattron(item_list[item_sel].window, A_REVERSE);
		mvwprintw(item_list[item_sel].window, 0, left_offset, "%s\n", item_image(item_list[item_sel].item));
		wattroff(item_list[item_sel].window, A_REVERSE);
		wrefresh(item_list[item_sel].window);
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
		if      (ch == 'j'  || ch == KEY_DOWN)          { item_select(item_sel + 1); }
		else if (ch == 'k'  || ch == KEY_UP)            { item_select(item_sel - 1); }
		else if (ch == 'l'  || ch == KEY_RIGHT ||
		         ch == '\n' || ch == KEY_ENTER)         { return MENU_CONTENTS; }
		/*else if (ch == 'd')                             { item_reload(&item_list[item_sel]); }*/
		/*else if (ch == 'D')                             { item_reload_all(); }*/
		else if (ch == 'g' && wgetch(input_win) == 'g') { item_select(0); }
		else if (ch == 'G')                             { item_select(item_count - 1); }
		else if (isdigit(ch)) {
			q = 0;
			while (1) {
				cmd[q++] = ch;
				if (q > 6) break;
				cmd[q] = '\0';
				ch = wgetch(input_win);
				if (!isdigit(ch)) {
					if (ch == 'j' || ch == KEY_DOWN) {
						item_select(item_sel + atoi(cmd));
					} else if (ch == 'k' || ch == KEY_UP) {
						item_select(item_sel - atoi(cmd));
					} else if (ch == 'G') {
						item_select(atoi(cmd) - 1);
					}
					break;
				}
			}
		}
		else if (ch == 'q') return MENU_EXIT;
	}
}
