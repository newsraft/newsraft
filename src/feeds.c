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
			if (feed_list[i].feed->name != NULL) free(feed_list[i].feed->name);
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
	int feed_index, letter, errors = 0;
	char c, *path;
	path = get_config_file_path("feeds");
	if (path == NULL) {
		++errors;
		fprintf(stderr, "could not find feeds file!\n");
		return errors;
	}
	FILE *f = fopen(path, "r");
	free(path);
	if (f == NULL) {
		++errors;
		fprintf(stderr, "could not open feeds file!\n");
		return errors;
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
			feed_list[feed_index].feed->name = malloc(sizeof(char) * MAX_NAME_SIZE);
			c = fgetc(f);
			while ((c != '\n') && (c != EOF)) { feed_list[feed_index].feed->name[letter++] = c; c = fgetc(f); }
			feed_list[feed_index].feed->name[letter] = '\0';
			continue;
		}
		feed_list[feed_index].feed->feed_url = malloc(sizeof(char) * MAX_URL_SIZE);
		while (1) {
			feed_list[feed_index].feed->feed_url[letter++] = c;
			c = fgetc(f);
			if (c == ' ' || c == '\t' || c == '\n' || c == EOF) {
				feed_list[feed_index].feed->feed_url[letter] = '\0';
				feed_list[feed_index].feed->path = make_feed_dir(feed_list[feed_index].feed->feed_url);
				if (feed_list[feed_index].feed->path == NULL) {
					++errors;
					fprintf(stderr, "could not set directory for \"%s\"!\n", feed_list[feed_index].feed->feed_url);
				} else {
					feed_list[feed_index].feed->site_url = export_feed_value(feed_list[feed_index].feed->path, "link");
				}
				break;
			}
		}
		if (c == EOF) break;
		if (c == '\n') continue;
		skip_chars(f, &c, " \t");
		if (c == '\n') continue;
		if (c == '"') {
			letter = 0;
			feed_list[feed_index].feed->name = malloc(sizeof(char) * MAX_NAME_SIZE);
			while (1) {
				c = fgetc(f);
				if (c == '"') { feed_list[feed_index].feed->name[letter] = '\0'; break; }
				feed_list[feed_index].feed->name[letter++] = c;
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
		++errors;
		free_feed_list();
		fprintf(stderr, "none of feeds loaded!\n");
		return errors;
	}

	return errors; // success
}

// return most sensible string for feed title
static char *
feed_image(struct feed_entry *feed) {
	return feed->name ? feed->name : feed->feed_url;
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
		feed_list[i].window = newwin(1, COLS, i + config_top_offset, config_left_offset);
		if (i == feed_sel) wattron(feed_list[i].window, A_REVERSE);
		mvwprintw(feed_list[i].window, 0, 0, "%s\n", feed_image(feed_list[i].feed));
		if (i == feed_sel) wattroff(feed_list[i].window, A_REVERSE);
		wrefresh(feed_list[i].window);
	}
	enum menu_dest dest = menu_feeds();
	hide_feeds();
	if (dest == MENU_ITEMS) {
		items_menu(feed_list[feed_sel].feed->path);
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
		mvwprintw(feed_list[feed_sel].window, 0, 0, "%s\n", feed_image(feed_list[feed_sel].feed));
		wrefresh(feed_list[feed_sel].window);
		feed_sel = new_sel;
		wattron(feed_list[feed_sel].window, A_REVERSE);
		mvwprintw(feed_list[feed_sel].window, 0, 0, "%s\n", feed_image(feed_list[feed_sel].feed));
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
		if      (ch == 'j') { feed_select(feed_sel + 1); }
		else if (ch == 'k') { feed_select(feed_sel - 1); }
		else if (ch == 'l') { return MENU_ITEMS; }
		else if (ch == 'h') { return MENU_EXIT; }
		else if (ch == 'd') { feed_reload(&feed_list[feed_sel]); }
		else if (ch == 'D') { feed_reload_all(); }
		else if (ch == 'g' && wgetch(input_win) == 'g') { feed_select(0); }
		else if (ch == 'G') { feed_select(feed_count - 1); }
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
	}
}

char *
export_feed_value(char *feed_path, char *element)
{
	char *path = malloc(sizeof(char) * MAXPATH);
	if (path == NULL) return NULL;
	strcpy(path, feed_path);
	strcat(path, "elements/");
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

void
write_feed_element(char *feed_path, char *element, void *data, size_t size)
{
	if (feed_path == NULL || data == NULL || size == 0) return;
	char *feed = malloc(sizeof(char) * MAXPATH);
	if (feed == NULL) return;
	strcpy(feed, feed_path);
	strcat(feed, "elements/");
	strcat(feed, element);
	FILE *f = fopen(feed, "w");
	free(feed);
	if (f == NULL) return;
	fwrite(data, size, 1, f);
	fclose(f);
}
