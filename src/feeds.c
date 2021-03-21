#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include "feedeater.h"
#define MAX_NAME_SIZE 128
#define MAX_URL_SIZE 128

struct feed_entry {
	char *name;
	char *url;
	WINDOW *window;
};

static struct feed_entry *feed_list = NULL;
static int feed_sel = -1;
static int feed_count = 0;

int
load_feeds(void)
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
		feed_list = realloc(feed_list, sizeof(struct feed_entry) * feed_count);
		if (c == '@') {
			feed_list[feed_index].name = malloc(sizeof(char) * MAX_NAME_SIZE);
			c = fgetc(f);
			while ((c != '\n') && (c != EOF)) { feed_list[feed_index].name[letter++] = c; c = fgetc(f); }
			feed_list[feed_index].name[letter] = '\0';
			feed_list[feed_index].url = NULL;
			continue;
		}
		feed_list[feed_index].name = NULL;
		feed_list[feed_index].url = malloc(sizeof(char) * MAX_URL_SIZE);
		while (1) {
			feed_list[feed_index].url[letter++] = c;
			c = fgetc(f);
			if (c == ' ' || c == '\t' || c == '\n' || c == EOF) {
				feed_list[feed_index].url[letter] = '\0';
				break;
			}
		}
		if (c != ' ' && c != '\t') continue;
		skip_chars(f, &c, " \t");
		if (c == EOF) break;
		if (c == '\n') continue;
		if (c == '"') {
			letter = 0;
			feed_list[feed_index].name = malloc(sizeof(char) * MAX_NAME_SIZE);
			while (1) {
				c = fgetc(f);
				if (c == '"') { feed_list[feed_index].name[letter] = '\0'; break; }
				feed_list[feed_index].name[letter++] = c;
			}
		}
		// move to the end of line or file
		while ((c != '\n') && (c != EOF)) c = fgetc(f);
	}
	fclose(f);

	// put first non-decorative item in selection
	for (feed_index = 0; feed_index < feed_count; ++feed_index) {
		if (feed_list[feed_index].url != NULL) {
			feed_sel = feed_index;
			break;
		}
	}

	if (feed_sel == -1) {
		fprintf(stderr, "none of feeds loaded!\n");
		return 0;
	}

	return 1;
}

void
close_feeds(void)
{
	for (int i = 0; i < feed_count; ++i) {
		if (feed_list[i].url != NULL) free(feed_list[i].url);
		if (feed_list[i].name != NULL) free(feed_list[i].name);
	}
	if (feed_list != NULL) free(feed_list);
}

void
show_feeds(void)
{
	clear();
	for (int i = 0; i < feed_count; ++i) {
		feed_list[i].window = newwin(1, COLS, i, 0);
		if (i == feed_sel) wattron(feed_list[i].window, A_REVERSE);
		mvwprintw(feed_list[i].window, 0, 0, "%s\n", feed_list[i].name ? feed_list[i].name : feed_list[i].url);
		if (i == feed_sel) wattroff(feed_list[i].window, A_REVERSE);
		wrefresh(feed_list[i].window);
	}
}

void
hide_feeds(void)
{
	for (int i = 0; i < feed_count; ++i) delwin(feed_list[i].window);
	clear();
}

void
feed_select(int i)
{
	int new_sel = i, j;
	if (new_sel < 0) {
		new_sel = 0;
	} else if (new_sel >= feed_count) {
		new_sel = feed_count - 1;
		if (new_sel < 0) return;
	}
	if (new_sel > feed_sel && feed_list[new_sel].url == NULL) {
		for (j = new_sel; (j < feed_count) && (feed_list[j].url == NULL); ++j);
		if (j < feed_count) {
			new_sel = j;
		} else {
			for (j = feed_sel; j < feed_count; ++j) {
				if (feed_list[j].url != NULL) new_sel = j;
			}
		}
	} else if (new_sel < feed_sel && feed_list[new_sel].url == NULL) {
		for (j = new_sel; (j > -1) && (feed_list[j].url == NULL); --j);
		if (j > -1) {
			new_sel = j;
		} else {
			for (j = feed_sel; j > -1; --j) {
				if (feed_list[j].url != NULL) new_sel = j;
			}
		}
	}
	if (feed_list[new_sel].url == NULL) return;
	if (new_sel != feed_sel) {
		mvwprintw(feed_list[feed_sel].window, 0, 0, "%s\n", feed_list[feed_sel].name ? feed_list[feed_sel].name : feed_list[feed_sel].url);
		wrefresh(feed_list[feed_sel].window);
		feed_sel = new_sel;
		wattron(feed_list[feed_sel].window, A_REVERSE);
		mvwprintw(feed_list[feed_sel].window, 0, 0, "%s\n", feed_list[feed_sel].name ? feed_list[feed_sel].name : feed_list[feed_sel].url);
		wattroff(feed_list[feed_sel].window, A_REVERSE);
		wrefresh(feed_list[feed_sel].window);
	}
}

void
menu_feeds(void)
{
	WINDOW *input_win = newwin(1,1,LINES,COLS);
	keypad(input_win, TRUE); // used to recognize arrow keys
	int ch, q;
	char cmd[7];
	while (1) {
		ch = wgetch(input_win);
		wrefresh(input_win);
		if      (ch == 'j'  || ch == KEY_DOWN)          { feed_select(feed_sel + 1); }
		else if (ch == 'k'  || ch == KEY_UP)            { feed_select(feed_sel - 1); }
		else if (ch == 'l'  || ch == KEY_RIGHT ||
		         ch == '\n' || ch == KEY_ENTER)         { feed_view(feed_list[feed_sel].url); }
		else if (ch == 'd')                             { feed_reload(feed_list[feed_sel].url); }
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
		else if (ch == 'q') break;
	}
	delwin(input_win);
}
