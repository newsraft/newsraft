#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "feedeater.h"
#include "config.h"

static WINDOW *window;
static int menu_contents(void);
static int view_min;
static int view_max;

static void
print_content(char *item_path)
{
	char *title = read_item_element(item_path, TITLE_FILE);
	if (title != NULL) {
		waddstr(window, "Title: "); waddstr(window, title); waddch(window, '\n');
		free(title);
	}
	char *date = read_item_element(item_path, PUBDATE_FILE);
	if (date != NULL) {
		waddstr(window, "Date: "); waddstr(window, date); waddch(window, '\n');
		free(date);
	}
	char *author = read_item_element(item_path, AUTHOR_FILE);
	if (author != NULL) {
		waddstr(window, "Author: "); waddstr(window, author); waddch(window, '\n');
		free(author);
	}
	char *link = read_item_element(item_path, LINK_FILE);
	if (link != NULL) {
		waddstr(window, "Link: "); waddstr(window, link); waddch(window, '\n');
		free(link);
	}
	char *content = read_item_element(item_path, CONTENT_FILE);
	if (content != NULL) {
		waddch(window, '\n');
		waddstr(window, content);
		/*char *saveptr;*/
		/*char *line = strtok_r(content, "\n", &saveptr);*/
		/*do {*/
			/*waddstr(window, line);*/
			/*waddch(window, '\n');*/
		/*} while ((line = strtok_r(NULL, "\n", &saveptr)) != NULL);*/
		free(content);
	}
}

int
contents_menu(char *feed_path, int64_t index)
{
	view_min = 0;
	view_max = LINES - 1;
	char *item_path = item_data_path(feed_path, index);
	clear();
	refresh();
	mark_read(item_path);
	window = newpad(LINES - 1, COLS);
	print_content(item_path);
	prefresh(window, 0, 0, 0, 0, LINES - 1, COLS);
	int contents_status = menu_contents();
	delwin(window);
	free(item_path);
	return contents_status;
}

void
scroll_view(int offset)
{
	int new_view_min = view_min + offset;
	int new_view_max = view_max + offset;
	if (new_view_min < 0) {
		new_view_min = 0;
		new_view_max = LINES - 1;
	}
	if (new_view_min != view_min && new_view_max != view_max) {
		view_min = new_view_min;
		view_max = new_view_max;
		prefresh(window, view_min, 0, 0, 0, LINES - 1, COLS);
	}
}

void
scroll_view_top(void)
{
	return;
}

void
scroll_view_bot(void)
{
	return;
}

static int
menu_contents(void)
{
	int ch, q;
	char cmd[7];
	while (1) {
		ch = wgetch(input_win);
		wrefresh(input_win);
		if      (ch == 'j' || ch == KEY_DOWN)              { scroll_view(1); }
		else if (ch == 'k' || ch == KEY_UP)                { scroll_view(-1); }
		else if (ch == 'h' || ch == KEY_LEFT)              { return MENU_ITEMS; }
		else if (ch == 'q')                                { return MENU_EXIT; }
		else if (ch == 'g' && wgetch(input_win) == 'g')    { scroll_view_top(); }
		else if (ch == 'G')                                { scroll_view_bot(); }
		else if (isdigit(ch)) {
			q = 0;
			while (1) {
				cmd[q++] = ch;
				if (q > 6) break;
				cmd[q] = '\0';
				ch = wgetch(input_win);
				/*if (!isdigit(ch)) {*/
					/*if (ch == 'j' || ch == KEY_DOWN) {*/
						/*feed_select(feed_sel + atoi(cmd));*/
					/*} else if (ch == 'k' || ch == KEY_UP) {*/
						/*feed_select(feed_sel - atoi(cmd));*/
					/*} else if (ch == 'G') {*/
						/*feed_select(atoi(cmd) - 1);*/
					/*}*/
					/*break;*/
				/*}*/
			}
		}
	}
}
