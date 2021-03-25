#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include "feedeater.h"
#define MAX_NAME_SIZE 128
#define MAX_URL_SIZE 128

static struct feed_window *feed_list = NULL;
static int feed_sel = -1;
static int feed_count = 0;

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
		feed_list[feed_index].feed = malloc(sizeof(struct feed_entry));
		if (c == '@') {
			feed_list[feed_index].feed->lname = malloc(sizeof(char) * MAX_NAME_SIZE);
			c = fgetc(f);
			while ((c != '\n') && (c != EOF)) { feed_list[feed_index].feed->lname[letter++] = c; c = fgetc(f); }
			feed_list[feed_index].feed->lname[letter] = '\0';
			feed_list[feed_index].feed->lurl = NULL;
			continue;
		}
		feed_list[feed_index].feed->lname = NULL;
		feed_list[feed_index].feed->lurl = malloc(sizeof(char) * MAX_URL_SIZE);
		while (1) {
			feed_list[feed_index].feed->lurl[letter++] = c;
			c = fgetc(f);
			if (c == ' ' || c == '\t' || c == '\n' || c == EOF) {
				feed_list[feed_index].feed->lurl[letter] = '\0';
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
		if (feed_list[feed_index].feed->lurl != NULL) {
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
free_feed_list(void)
{
	if (feed_list == NULL) return;
	for (int i = 0; i < feed_count; ++i) {
		if (feed_list[i].feed != NULL) {
			if (feed_list[i].feed->lurl != NULL) free(feed_list[i].feed->lurl);
			if (feed_list[i].feed->lname != NULL) free(feed_list[i].feed->lname);
			if (feed_list[i].feed->rurl != NULL) free(feed_list[i].feed->rurl);
			if (feed_list[i].feed->rname != NULL) free(feed_list[i].feed->rname);
			free(feed_list[i].feed);
		}
		// core dumped with this :(
		/*if (feed_list[i].window != NULL) {*/
			/*free(feed_list[i].window);*/
		/*}*/
	}
	feed_sel = -1;
	feed_count = 0;
	free(feed_list);
}

void
show_feeds(void)
{
	clear();
	for (int i = 0; i < feed_count; ++i) {
		feed_list[i].window = newwin(1, COLS, i, 0);
		if (i == feed_sel) wattron(feed_list[i].window, A_REVERSE);
		mvwprintw(feed_list[i].window, 0, 0, "%s\n", feed_image(feed_list[i].feed));
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
	if (new_sel > feed_sel && feed_list[new_sel].feed->lurl == NULL) {
		for (j = new_sel; (j < feed_count) && (feed_list[j].feed->lurl == NULL); ++j);
		if (j < feed_count) {
			new_sel = j;
		} else {
			for (j = feed_sel; j < feed_count; ++j) {
				if (feed_list[j].feed->lurl != NULL) new_sel = j;
			}
		}
	} else if (new_sel < feed_sel && feed_list[new_sel].feed->lurl == NULL) {
		for (j = new_sel; (j > -1) && (feed_list[j].feed->lurl == NULL); --j);
		if (j > -1) {
			new_sel = j;
		} else {
			for (j = feed_sel; j > -1; --j) {
				if (feed_list[j].feed->lurl != NULL) new_sel = j;
			}
		}
	}
	if (feed_list[new_sel].feed->lurl == NULL) return;
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
		         ch == '\n' || ch == KEY_ENTER)         { feed_view(feed_list[feed_sel].feed->lurl); }
		else if (ch == 'd')                             { feed_reload(feed_list[feed_sel].feed); }
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

char *
feed_image(struct feed_entry *feed) {
	return feed->lname ? feed->lname : (feed->rname ? feed->rname : feed->lurl);
}

void
free_feed_entry(struct feed_entry *feed)
{
	if (feed == NULL) return;
	if (feed->lname != NULL) free(feed->lname);
	if (feed->rname != NULL) free(feed->rname);
	if (feed->lurl != NULL) free(feed->lurl);
	if (feed->rurl != NULL) free(feed->rurl);
	free(feed);
}
