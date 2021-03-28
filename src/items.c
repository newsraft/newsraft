#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"
#include "config.h"

static struct item_window *item_list = NULL;
static int item_sel = -1;
static int item_count = 0;

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
	int border_index = get_last_item_index(data_path);
	bool past_line = false;
	for (int i = border_index + 1; ; ++i) {
		if (past_line == false && i > config_max_items) {
			past_line = true;
			i = 1;
		}
		if (past_line == true && i == border_index + 1) {
			break;
		}
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
		return 1; // no items found
	}

	return 0;
}

void
hide_items(void)
{
	for (int i = 0; i < item_count; ++i) {
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
			free_item_list();
			return load_status;
		}
	}

	clear();
	refresh();
	for (int i = 0; i < item_count; ++i) {
		item_list[i].window = newwin(1, COLS, i + config_top_offset, config_left_offset);
		if (i == item_sel) wattron(item_list[i].window, A_REVERSE);
		mvwprintw(item_list[i].window, 0, 0, "%s\n", item_image(item_list[i].item));
		if (i == item_sel) wattroff(item_list[i].window, A_REVERSE);
		wrefresh(item_list[i].window);
	}

	int dest = menu_items();
	hide_items();
	if (dest == MENU_CONTENTS) {
		contents_menu();
	} else if (dest == MENU_EXIT) {
		free_item_list();
		return 1;
	}

	items_menu(data_path);
	return 1;
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
		mvwprintw(item_list[item_sel].window, 0, 0, "%s\n", item_image(item_list[item_sel].item));
		wrefresh(item_list[item_sel].window);
		item_sel = new_sel;
		wattron(item_list[item_sel].window, A_REVERSE);
		mvwprintw(item_list[item_sel].window, 0, 0, "%s\n", item_image(item_list[item_sel].item));
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
		if      (ch == 'j' || ch == KEY_DOWN)                                   { item_select(item_sel + 1); }
		else if (ch == 'k' || ch == KEY_UP)                                     { item_select(item_sel - 1); }
		else if (ch == 'l' || ch == KEY_RIGHT || ch == '\n' || ch == KEY_ENTER) { return MENU_CONTENTS; }
		else if (ch == 'h' || ch == KEY_LEFT)                                   { return MENU_EXIT; }
		else if (ch == 'G')                                                     { item_select(item_count - 1); }
		else if (ch == 'g' && wgetch(input_win) == 'g')                         { item_select(0); }
		else if (isdigit(ch)) {
			q = 0;
			while (1) {
				cmd[q++] = ch;
				if (q > 6) break;
				cmd[q] = '\0';
				ch = wgetch(input_win);
				if (!isdigit(ch)) {
					if (ch == 'j') {
						item_select(item_sel + atoi(cmd));
					} else if (ch == 'k') {
						item_select(item_sel - atoi(cmd));
					} else if (ch == 'G') {
						item_select(atoi(cmd) - 1);
					}
					break;
				}
			}
		} 
	}
}

char *
item_data_path(char *feed_path, int num)
{
	if (feed_path == NULL) return NULL;
	char *path = malloc(sizeof(char) * MAXPATH);
	if (path == NULL) return NULL;
	strcpy(path, feed_path);
	char dir_name[MAX_ITEM_INDEX_LEN];
	sprintf(dir_name, "%d", num);
	strcat(path, dir_name);
	strcat(path, "/");
	mkdir(path, 0777);
	return path;
}

void
set_last_item_index(char *feed_path, int index)
{
	char *path = malloc(sizeof(char) * MAXPATH);
	if (path == NULL) {
		status_write("not enough ram");
		return;
	}
	strcpy(path, feed_path);
	strcat(path, "last_item");
	FILE *f = fopen(path, "w");
	free(path);
	if (f == NULL) return;
	fprintf(f, "%d", index);
	fclose(f);
}

int
get_last_item_index(char *feed_path)
{
	char *path = malloc(sizeof(char) * MAXPATH);
	if (path == NULL) {
		status_write("not enough ram");
		return 0;
	}
	strcpy(path, feed_path);
	strcat(path, "last_item");
	FILE *f = fopen(path, "r");
	free(path);
	if (f == NULL) { return 0; }
	int i = 0;
	char index[MAX_ITEM_INDEX_LEN], c;
	while ((c = fgetc(f)) != EOF) index[i++] = c;
	index[i] = '\0';
	fclose(f);
	return atoi(index);
}

void
write_item_element(char *item_path, char *element, void *data, size_t size)
{
	if (item_path == NULL || data == NULL || size == 0) return;
	char *item = malloc(sizeof(char) * MAXPATH);
	if (item == NULL) return;
	strcpy(item, item_path);
	strcat(item, element);
	FILE *f = fopen(item, "w");
	free(item);
	if (f == NULL) return;
	fwrite(data, size, 1, f);
	fclose(f);
}
