#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"
#include "config.h"
#define MAX_NAME_SIZE 128
#define MAX_URL_SIZE 128

static enum menu_dest menu_feeds(void);
static struct feed_window *feed_list = NULL;
static int feed_sel = -1;
static int feed_count = 0;

static void
free_feed_list(void)
{
	if (feed_list == NULL) return;
	for (int i = 0; i < feed_count; ++i) {
		if (feed_list[i].feed != NULL) {
			if (feed_list[i].feed->lname != NULL) free(feed_list[i].feed->lname);
			if (feed_list[i].feed->rname != NULL) free(feed_list[i].feed->rname);
			if (feed_list[i].feed->feed_url != NULL) free(feed_list[i].feed->feed_url);
			if (feed_list[i].feed->site_url != NULL) free(feed_list[i].feed->site_url);
			free(feed_list[i].feed);
		}
	}
	feed_sel = -1;
	feed_count = 0;
	free(feed_list);
	feed_list = NULL;
}

int
load_feed_list(void)
{
	int feed_index, letter;
	char c, *path;
	path = get_config_file_path("feeds");
	if (path == NULL) {
		fprintf(stderr, "could not find feeds file!\n");
		return 0;
	}
	FILE *f = fopen(path, "r");
	free(path);
	if (f == NULL) {
		fprintf(stderr, "could not open feeds file!\n");
		return 0;
	}
	while (1) {
		c = fgetc(f);
		skip_chars(f, &c, " \t\n");
		if (c == EOF) break;
		if (c == '#') {
			do { c = fgetc(f); } while ((c != '\n') && (c != EOF)); continue;
		}
		letter = 0;
		feed_index = feed_count++;
		feed_list = realloc(feed_list, sizeof(struct feed_window) * feed_count);
		feed_list[feed_index].feed = calloc(1, sizeof(struct feed_entry));
		if (c == '@') {
			feed_list[feed_index].feed->lname = malloc(sizeof(char) * MAX_NAME_SIZE);
			c = fgetc(f);
			while ((c != '\n') && (c != EOF)) { feed_list[feed_index].feed->lname[letter++] = c; c = fgetc(f); }
			feed_list[feed_index].feed->lname[letter] = '\0';
			continue;
		}
		feed_list[feed_index].feed->feed_url = malloc(sizeof(char) * MAX_URL_SIZE);
		while (1) {
			feed_list[feed_index].feed->feed_url[letter++] = c;
			c = fgetc(f);
			if (c == ' ' || c == '\t' || c == '\n' || c == EOF) {
				feed_list[feed_index].feed->feed_url[letter] = '\0';
				feed_list[feed_index].feed->rname = export_feed_value(feed_list[feed_index].feed->feed_url, "title");
				feed_list[feed_index].feed->site_url = export_feed_value(feed_list[feed_index].feed->feed_url, "link");
				break;
			}
		}
		if (c != ' ' && c != '\t') continue;
		skip_chars(f, &c, " \t");
		if (c == EOF) break;
		if (c == '\n') continue;
		if (c == '"') {
			letter = 0;
			feed_list[feed_index].feed->lname = malloc(sizeof(char) * MAX_NAME_SIZE);
			while (1) {
				c = fgetc(f);
				if (c == '"') { feed_list[feed_index].feed->lname[letter] = '\0'; break; }
				feed_list[feed_index].feed->lname[letter++] = c;
			}
		}
		// move to the end of line or file
		while ((c != '\n') && (c != EOF)) c = fgetc(f);
	}
	fclose(f);

	// put first non-decorative item in selection
	for (feed_index = 0; feed_index < feed_count; ++feed_index) {
		if (feed_list[feed_index].feed->feed_url != NULL) {
			feed_sel = feed_index;
			break;
		}
	}

	if (feed_sel == -1) {
		free_feed_list();
		fprintf(stderr, "none of feeds loaded!\n");
		return 0;
	}

	return 1; // success
}

// return most sensible string for feed title
static char *
feed_image(struct feed_entry *feed) {
	return feed->lname ? feed->lname : (feed->rname ? feed->rname : feed->feed_url);
}

void
hide_feeds(void)
{
	for (int i = 0; i < feed_count; ++i) {
		if (feed_list[i].window != NULL) {
			delwin(feed_list[i].window);
		}
	}
}

void
feeds_menu(void)
{
	clear();
	refresh();
	for (int i = 0; i < feed_count; ++i) {
		feed_list[i].window = newwin(1, COLS, i, 0);
		if (i == feed_sel) wattron(feed_list[i].window, A_REVERSE);
		mvwprintw(feed_list[i].window, 0, left_offset, "%s\n", feed_image(feed_list[i].feed));
		if (i == feed_sel) wattroff(feed_list[i].window, A_REVERSE);
		wrefresh(feed_list[i].window);
	}
	enum menu_dest dest = menu_feeds();
	hide_feeds();
	if (dest == MENU_ITEMS) {
		items_menu(feed_list[feed_sel].feed->feed_url);
	} else if (dest == MENU_EXIT) {
		free_feed_list();
		return;
	}
	feeds_menu();
}

static void
feed_select(int i)
{
	int new_sel = i, j;
	if (new_sel < 0) {
		new_sel = 0;
	} else if (new_sel >= feed_count) {
		new_sel = feed_count - 1;
		if (new_sel < 0) return;
	}
	if (new_sel > feed_sel && feed_list[new_sel].feed->feed_url == NULL) {
		for (j = new_sel; (j < feed_count) && (feed_list[j].feed->feed_url == NULL); ++j);
		if (j < feed_count) {
			new_sel = j;
		} else {
			for (j = feed_sel; j < feed_count; ++j) {
				if (feed_list[j].feed->feed_url != NULL) new_sel = j;
			}
		}
	} else if (new_sel < feed_sel && feed_list[new_sel].feed->feed_url == NULL) {
		for (j = new_sel; (j > -1) && (feed_list[j].feed->feed_url == NULL); --j);
		if (j > -1) {
			new_sel = j;
		} else {
			for (j = feed_sel; j > -1; --j) {
				if (feed_list[j].feed->feed_url != NULL) new_sel = j;
			}
		}
	}
	if (feed_list[new_sel].feed->feed_url == NULL) return;
	if (new_sel != feed_sel) {
		mvwprintw(feed_list[feed_sel].window, 0, left_offset, "%s\n", feed_image(feed_list[feed_sel].feed));
		wrefresh(feed_list[feed_sel].window);
		feed_sel = new_sel;
		wattron(feed_list[feed_sel].window, A_REVERSE);
		mvwprintw(feed_list[feed_sel].window, 0, left_offset, "%s\n", feed_image(feed_list[feed_sel].feed));
		wattroff(feed_list[feed_sel].window, A_REVERSE);
		wrefresh(feed_list[feed_sel].window);
	}
}

static void
feed_reload(struct feed_window *feedwin)
{
	struct string *buf = feed_download(feedwin->feed->feed_url);
	if (buf == NULL) return;
	if (buf->ptr == NULL) { free(buf); return; }
	feed_process(buf, feedwin->feed);
	free(buf->ptr);
	free(buf);
}

static void
feed_reload_all(void)
{
	//under construction
	return;
}

static enum menu_dest
menu_feeds(void)
{
	int ch, q;
	char cmd[7];
	while (1) {
		ch = wgetch(input_win);
		wrefresh(input_win);
		if      (ch == 'j'  || ch == KEY_DOWN)          { feed_select(feed_sel + 1); }
		else if (ch == 'k'  || ch == KEY_UP)            { feed_select(feed_sel - 1); }
		else if (ch == 'l'  || ch == KEY_RIGHT ||
		         ch == '\n' || ch == KEY_ENTER)         { return MENU_ITEMS; }
		else if (ch == 'd')                             { feed_reload(&feed_list[feed_sel]); }
		else if (ch == 'D')                             { feed_reload_all(); }
		else if (ch == 'g' && wgetch(input_win) == 'g') { feed_select(0); }
		else if (ch == 'G')                             { feed_select(feed_count - 1); }
		else if (isdigit(ch)) {
			q = 0;
			while (1) {
				cmd[q++] = ch;
				if (q > 6) break;
				cmd[q] = '\0';
				ch = wgetch(input_win);
				if (!isdigit(ch)) {
					if (ch == 'j' || ch == KEY_DOWN) {
						feed_select(feed_sel + atoi(cmd));
					} else if (ch == 'k' || ch == KEY_UP) {
						feed_select(feed_sel - atoi(cmd));
					} else if (ch == 'G') {
						feed_select(atoi(cmd) - 1);
					}
					break;
				}
			}
		}
		else if (ch == 'q') return MENU_EXIT;
	}
}


void
free_feed_entry(struct feed_entry *feed)
{
	if (feed == NULL) return;
	if (feed->lname != NULL) free(feed->lname);
	if (feed->rname != NULL) free(feed->rname);
	if (feed->feed_url != NULL) free(feed->feed_url);
	if (feed->site_url != NULL) free(feed->site_url);
	free(feed);
	feed = NULL;
}

int
import_feed_value(char *url, char *element, char *value)
{
	char *path = get_data_dir_path_for_url(url);
	if (path == NULL) return 0;
	strcat(path, "/elements/");
	mkdir(path, 0777);
	strcat(path, element);
	FILE *f = fopen(path, "w");
	free(path);
	if (f == NULL) return 0;
	fputs(value, f);
	fclose(f);
	return 1; // success
}

char *
export_feed_value(char *url, char *element)
{
	char *path = get_data_dir_path_for_url(url);
	if (path == NULL) return NULL;
	strcat(path, "/elements/");
	strcat(path, element);
	FILE *f = fopen(path, "r");
	free(path);
	if (f == NULL) return NULL;
	int size = 32, count = 0;
	char *value = malloc(sizeof(char) * size);
	char c;
	while ((c = fgetc(f)) != EOF) {
		value[count++] = c;
		if (count == size) {
			size *= 2;
			value = realloc(value, sizeof(char) * size);
		}
	}
	value[count] = '\0';
	fclose(f);
	return value;
}

char *
feed_item_data_path(char *url, int num)
{
	char *path = get_data_dir_path_for_url(url);
	if (path == NULL) return NULL;
	char dir_name[6];
	sprintf(dir_name, "%d", num);
	strcat(path, dir_name);
	strcat(path, "/");
	mkdir(path, 0777);
	return path;
}

void
write_feed_item_elem(char *item_path, char *item_name, void *data, size_t size)
{
	char *item = malloc(sizeof(char) * MAXPATH);
	if (item == NULL) return;
	strcpy(item, item_path);
	strcat(item, item_name);
	FILE *f = fopen(item, "w");
	fwrite(data, size, 1, f);
	fclose(f);
}
