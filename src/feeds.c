#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include "feedeater.h"
#define MAX_NAME_SIZE 128
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
	int feed_index, letter;
	char c, *path, word[40];
	path = get_config_file_path("feeds");
	if (path == NULL) {
		fprintf(stderr, "could not find feeds file!\n");
		return 0;
	}
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		fprintf(stderr, "could not open feeds file!\n");
		free(path);
		return 0;
	}
	while (1) {
		c = fgetc(f);
		skip_chars(f, &c, " \n");
		if (c == EOF) break;
		feed_index = feed_count++;
		feed_list = realloc(feed_list, sizeof(struct feed_entry) * feed_count);
		feed_list[feed_index].name = NULL;
		feed_list[feed_index].url = malloc(sizeof(char) * MAX_URL_SIZE);
		feed_list[feed_index].capacity = DEFAULT_CAPACITY;
		letter = 0;
		while (1) {
			feed_list[feed_index].url[letter++] = c;
			c = fgetc(f);
			if (c == ' ' || c == '\n' || c == EOF) {
				feed_list[feed_index].url[letter] = '\0';
				break;
			}
		}
		if (c != ' ') continue;
		skip_chars(f, &c, " ");
		if (c == '\n' || c == EOF) break;
		if (c == '"') {
			letter = 0;
			feed_list[feed_index].name = malloc(sizeof(char) * MAX_NAME_SIZE);
			while (1) {
				c = fgetc(f);
				if (c == '"') {
					feed_list[feed_index].name[letter] = '\0';
					break;
				}
				feed_list[feed_index].name[letter++] = c;
			}
		}
		/*letter = 0;*/
		/*while (1) {*/
			/*word[letter++] = c;*/
			/*c = fgetc(f);*/
			/*if (c == ' ' || c == '\n' || c == EOF) {*/
				/*word[letter] = '\0';*/
				/*break;*/
			/*}*/
		/*}*/
	}
	fclose(f);
	free(path);
	return 1;
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
close_feeds(void)
{
	for (int i = 0; i < feed_count; ++i) {
		if (feed_list[i].url != NULL) free(feed_list[i].url);
		if (feed_list[i].name != NULL) free(feed_list[i].name);
	}
	free(feed_list);
}

void
feed_select(int i)
{
	int new_sel = i;
	if (new_sel < 0) {
		new_sel = 0;
	} else if (new_sel >= feed_count) {
		new_sel = feed_count - 1;
	}
	if (new_sel == -1) return;
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
	WINDOW *input_win = newwin(1,COLS,LINES,0);
	keypad(input_win, TRUE); // used to recognize arrow keys
	int ch, q;
	char cmd[7];
	while (1) {
		ch = wgetch(input_win);
		wrefresh(input_win);
		if      (ch == 'j' || ch == KEY_DOWN)           { feed_select(feed_sel + 1); }
		else if (ch == 'k' || ch == KEY_UP)             { feed_select(feed_sel - 1); }
		else if (ch == 'G')                             { feed_select(feed_count - 1); }
		else if (ch == 'g' && wgetch(input_win) == 'g') { feed_select(0); }
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
