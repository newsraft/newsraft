#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "feedeater.h"
#include "config.h"

static WINDOW *window;
static int menu_contents(void);
static int view_min;
static int view_max;
static int newlines;

static struct string *
cat_content(char *item_path)
{
	struct string *buf = calloc(1, sizeof(struct string));
	if (buf == NULL) return NULL;
	buf->len = 1;
	buf->ptr = malloc(sizeof(char) * buf->len);
	if (buf->ptr == NULL) { free(buf); return NULL; }
	buf->ptr[0] = '\0';
#define BUF_APPEND(X, Y) if ((str = read_item_element(item_path, X)) != NULL) { \
                         	cat_string_cstr(buf, Y); cat_strings(buf, str); cat_string_cstr(buf, "\n"); \
                         	free_string_ptr(str); \
                         }
	struct string *str;
	BUF_APPEND(TITLE_FILE, "Title: ")
	BUF_APPEND(PUBDATE_FILE, "Date: ")
	BUF_APPEND(AUTHOR_FILE, "Author: ")
	BUF_APPEND(LINK_FILE, "Link: ")
	BUF_APPEND(COMMENTS_FILE, "Comments: ")
	struct string *content = read_item_element(item_path, CONTENT_FILE);
	if (content != NULL) {
		cat_string_cstr(buf, "\n");
		cat_strings(buf, content);
		free_string_ptr(content);
	}
	int not_newline = 0;
	long int c = 0;
	while (c < buf->len) {
		if ((buf->ptr)[c++] == '\n') {
			not_newline = 0;
			++newlines;
		} else {
			++not_newline;
			if (not_newline > COLS + 100) {
				not_newline = 0;
				++newlines;
			}
		}
	}
	return buf;
}

int
contents_menu(char *feed_path, int64_t index)
{
	newlines = 0;
	view_min = 0;
	view_max = LINES - 1;
	char *item_path = item_data_path(feed_path, index);
	if (item_path == NULL) {
		status_write("could not get path to this item");
		return MENU_CONTENT_ERROR;
	}
	struct string *content = cat_content(item_path);
	if (content == NULL) {
		status_write("could not obtain contents of item");
		free(item_path);
		return MENU_CONTENT_ERROR;
	}
	if (newlines == 0) {
		status_write("this item seems empty");
		free_string_ptr(content);
		free(item_path);
		return MENU_CONTENT_ERROR;
	}
	clear();
	refresh();
	mark_read(item_path);
	window = newpad(newlines, COLS);
	waddstr(window, content->ptr);
	free_string_ptr(content);
	prefresh(window, 0, 0, 0, 0, LINES - 2, COLS);
	int contents_status = menu_contents();
	delwin(window);
	free(item_path);
	return contents_status;
}

static void
scroll_view(int offset)
{
	int new_view_min = view_min + offset;
	int new_view_max = view_max + offset;
	if (new_view_min < 0) {
		new_view_min = 0;
		new_view_max = LINES - 1;
	}
	if (new_view_max >= newlines) {
		new_view_max = newlines - 1;
		new_view_min = newlines - LINES;
	}
	if (new_view_min != view_min && new_view_max != view_max) {
		view_min = new_view_min;
		view_max = new_view_max;
		prefresh(window, view_min, 0, 0, 0, LINES - 2, COLS);
	}
}

static void
scroll_view_top(void)
{
	int new_view_min = 0;
	int new_view_max = LINES - 1;
	if (new_view_min != view_min && new_view_max != view_max) {
		view_min = new_view_min;
		view_max = new_view_max;
		prefresh(window, view_min, 0, 0, 0, LINES - 2, COLS);
	}
}

static void
scroll_view_bot(void)
{
	int new_view_max = newlines - 1;
	int new_view_min = newlines - LINES;
	if (new_view_min != view_min && new_view_max != view_max) {
		view_min = new_view_min;
		view_max = new_view_max;
		prefresh(window, view_min, 0, 0, 0, LINES - 2, COLS);
	}
}

static int
menu_contents(void)
{
	int ch, q;
	char cmd[7];
	while (1) {
		ch = wgetch(input_win);
		wrefresh(input_win);
		if      (ch == 'j' || ch == KEY_DOWN)           { scroll_view(1); }
		else if (ch == 'k' || ch == KEY_UP)             { scroll_view(-1); }
		else if (ch == 'h' || ch == KEY_LEFT)           { return MENU_ITEMS; }
		else if (ch == 'g' && wgetch(input_win) == 'g') { scroll_view_top(); }
		else if (ch == 'G')                             { scroll_view_bot(); }
		else if (ch == config_key_exit)                 { return MENU_EXIT; }
		else if (isdigit(ch)) {
			q = 0;
			while (1) {
				cmd[q++] = ch;
				if (q > 6) break;
				cmd[q] = '\0';
				ch = wgetch(input_win);
				if (!isdigit(ch)) {
					if (ch == 'j' || ch == KEY_DOWN) {
						scroll_view(atoi(cmd));
					} else if (ch == 'k' || ch == KEY_UP) {
						scroll_view(-atoi(cmd));
					}
					break;
				}
			}
		}
	}
}
