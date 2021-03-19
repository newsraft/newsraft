#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include "feedeater.h"
#define MAX_URL_SIZE 128
#define DEFAULT_CAPACITY 10
struct feed_entry {
	char *name;
	char *url;
	int capacity;
	WINDOW *window;
};
static struct feed_entry *feed_list = NULL;
static int feed_sel = 0;
static int feed_count = 0;
void skip_chars(FILE *file, char *cur_char, char *list);

int
load_feeds(void)
{
	int feed_index = feed_count, letter;
	char c, *path;
	path = get_config_file_path("feeds");
	FILE *f = fopen(path, "r");
	while (1) {
		c = fgetc(f);
		skip_chars(f, &c, " \n");
		if (c == EOF) break;
		letter = 0;
		feed_index = feed_count++;
		feed_list = realloc(feed_list, sizeof(struct feed_entry) * feed_count);
		feed_list[feed_index].name = NULL;
		feed_list[feed_index].url = malloc(sizeof(char) * MAX_URL_SIZE);
		feed_list[feed_index].capacity = DEFAULT_CAPACITY;
		while (1) {
			feed_list[feed_index].url[letter++] = c;
			c = fgetc(f);
			if (c == ' ' || c == '\n' || c == EOF) {
				feed_list[feed_index].url[letter] = '\0';
				break;
			}
		}
	}
	fclose(f);
	free(path);
	return 0;
}

int
show_feeds(void)
{
	clear();
	for (int i = 0; i < feed_count; ++i) {
		feed_list[i].window = newwin(1, COLS, i, 0);
		if (i == feed_sel) wattron(feed_list[i].window, A_REVERSE);
		mvwprintw(feed_list[i].window, 0, 0, "%s\n", feed_list[i].url);
		if (i == feed_sel) wattroff(feed_list[i].window, A_REVERSE);
		wrefresh(feed_list[i].window);
	}
	return 0;
}

int
close_feeds(void)
{
	free(feed_list);
	return 0;
}

int
feed_select(int i)
{
	int new_sel = i;
	if (new_sel < 0) {
		new_sel = 0;
	} else if (new_sel >= feed_count) {
		new_sel = feed_count - 1;
	}
	if (new_sel == -1) return 0;
	if (new_sel != feed_sel) {
		mvwprintw(feed_list[feed_sel].window, 0, 0, "%s\n", feed_list[feed_sel].url);
		wrefresh(feed_list[feed_sel].window);
		feed_sel = new_sel;
		wattron(feed_list[feed_sel].window, A_REVERSE);
		mvwprintw(feed_list[feed_sel].window, 0, 0, "%s\n", feed_list[feed_sel].url);
		wattroff(feed_list[feed_sel].window, A_REVERSE);
		wrefresh(feed_list[feed_sel].window);
	}
	return 0;
}

int
menu_feeds(void)
{
	WINDOW *input_win = newwin(1,COLS,LINES,0);
	keypad(input_win, TRUE); // used to recognize arrow keys
	int ch, q;
	char cmd[100];
	while (1) {
		ch = wgetch(input_win);
		wrefresh(input_win);
		if      (ch == 'j' || ch == KEY_DOWN) { feed_select(feed_sel + 1); }
		else if (ch == 'k' || ch == KEY_UP)   { feed_select(feed_sel - 1); }
		else if (isdigit(ch)) {
			q = 0;
			while (1) {
				cmd[q++] = ch;
				cmd[q] = '\0';
				ch = wgetch(input_win);
				if (!isdigit(ch)) {
					if (ch == 'j' || ch == KEY_DOWN) {
						feed_select(feed_sel + atoi(cmd));
					} else if (ch == 'k' || ch == KEY_UP) {
						feed_select(feed_sel - atoi(cmd));
					} else if (ch == 'G') {
						feed_select(atoi(cmd));
					}
					break;
				}
			}
		}
		else if (ch == 'q') break;
	}
	delwin(input_win);
	return 0;
}

void
skip_chars(FILE *file, char *cur_char, char *list)
{
	uint8_t i = 0;
	while (list[i] != '\0') {
		if (*cur_char == list[i]) {
			*cur_char = fgetc(file);
			i = 0;
		} else {
			++i;
		}
	}
}
