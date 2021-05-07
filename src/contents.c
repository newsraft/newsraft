#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "feedeater.h"
#include "config.h"

static WINDOW *window;
static int view_min;
static int view_max;
static int newlines;

#define TEXT_TAG_APPEND(A, B) \
	text = (char *)sqlite3_column_text(res, B); \
	if (text != NULL) { \
		cat_string_array(buf, A); \
		cat_string_array(buf, text); \
		cat_string_char(buf, '\n'); \
		++newlines; \
	}
static struct string *
cat_content(struct string *feed_url, struct item_entry *item)
{
	struct string *buf = create_string();
	if (buf == NULL) return NULL;
	sqlite3_stmt *res;
	char *text, cmd[] = "SELECT * FROM items WHERE feed = ? AND guid = ? AND url = ? LIMIT 1";
	int rc = sqlite3_prepare_v2(db, cmd, -1, &res, 0);
	if (rc == SQLITE_OK) {
		sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
		sqlite3_bind_text(res, 2, item->guid->ptr, item->guid->len, NULL);
		sqlite3_bind_text(res, 3, item->url->ptr, item->url->len, NULL);
		rc = sqlite3_step(res);
		if (rc == SQLITE_ROW) {
			if (config_contents_show_feed == true)     { TEXT_TAG_APPEND("Feed: ", ITEM_COLUMN_FEED) }
			if (config_contents_show_title == true)    { TEXT_TAG_APPEND("Title: ", ITEM_COLUMN_TITLE) }
			if (config_contents_show_author == true)   { TEXT_TAG_APPEND("Author: ", ITEM_COLUMN_AUTHOR) }
			if (config_contents_show_category == true) { TEXT_TAG_APPEND("Category: ", ITEM_COLUMN_CATEGORY) }
			if (config_contents_show_comments == true) { TEXT_TAG_APPEND("Comments: ", ITEM_COLUMN_COMMENTS) }
			if (config_contents_show_date == true) {
				time_t epoch_time = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_PUBDATE);
				if (epoch_time > 0) {
					struct tm ts = *localtime(&epoch_time);
					char time_str[100];
					if (strftime(time_str, sizeof(time_str), config_contents_date_format, &ts) != 0) {
						cat_string_array(buf, "Date: ");
						cat_string_array(buf, time_str);
						cat_string_char(buf, '\n');
					}
				}
			}
			if (config_contents_show_url == true)      { TEXT_TAG_APPEND("Link: ", ITEM_COLUMN_URL) }
			if ((text = (char *)sqlite3_column_text(res, ITEM_COLUMN_CONTENT)) != NULL) {
				cat_string_char(buf, '\n');
				cat_string_array(buf, text);
			}
			uint16_t not_newline = 0;
			uint64_t i = 0;
			while (i <= buf->len) {
				if ((buf->ptr)[i++] == '\n') {
					not_newline = 0;
					++newlines;
				} else {
					++not_newline;
					if (not_newline > COLS) {
						not_newline = 0;
						++newlines;
					}
				}
			}
		} else {
			fprintf(stderr, "could not find that item\n");
			free_string(&buf);
		}
	} else {
		fprintf(stderr, "failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
		free_string(&buf);
	}
	sqlite3_finalize(res);
	return buf;
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
		ch = input_wgetch();
		if      (ch == 'j' || ch == KEY_DOWN)        { scroll_view(1); }
		else if (ch == 'k' || ch == KEY_UP)          { scroll_view(-1); }
		else if (ch == 'h' || ch == KEY_LEFT)        { return MENU_ITEMS; }
		else if (ch == 'g' && input_wgetch() == 'g') { scroll_view_top(); }
		else if (ch == 'G')                          { scroll_view_bot(); }
		else if (ch == config_key_exit)              { return MENU_EXIT; }
		else if (isdigit(ch)) {
			q = 0;
			while (1) {
				cmd[q++] = ch;
				if (q > 6) break;
				cmd[q] = '\0';
				ch = input_wgetch();
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

int
contents_menu(struct string *feed_url, struct item_entry *item)
{
	newlines = 0;
	view_min = 0;
	view_max = LINES - 1;
	struct string *content = cat_content(feed_url, item);
	if (content == NULL) {
		status_write("could not obtain contents of item");
		return MENU_CONTENT_ERROR;
	}
	if (newlines == 0) {
		status_write("this item seems empty");
		free_string(&content);
		return MENU_CONTENT_ERROR;
	}
	db_change_item_int(feed_url, item, ITEM_UNREAD_STATE, 0);
	hide_items();
	clear();
	refresh();
	window = newpad(newlines, COLS);
	waddstr(window, content->ptr);
	free_string(&content);
	prefresh(window, 0, 0, 0, 0, LINES - 2, COLS);
	int contents_status = menu_contents();
	delwin(window);
	return contents_status;
}
