#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "feedeater.h"
#include "config.h"

static WINDOW *window;
static int view_min;
static int view_max;
static int newlines;

#define TEXT_TAG_APPEND(A, B, C, D) \
	if (D == true) { \
		text = (char *)sqlite3_column_text(res, C); \
		if (text != NULL) { \
			text_len = strlen(text); \
			if (text_len != 0) { \
				cat_string_array(buf, A, (size_t)B); \
				cat_string_array(buf, text, text_len); \
				cat_string_char(buf, '\n'); \
				++pad_height; \
			} \
		} \
	}
static WINDOW *
cat_content(struct string *feed_url, struct item_entry *item)
{
	struct string *buf = create_string();
	if (buf == NULL) return NULL;
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(db, "SELECT * FROM items WHERE feed = ? AND guid = ? AND url = ? LIMIT 1", -1, &res, 0);
	if (rc != SQLITE_OK) {
		debug_write(DBG_WARN, "failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
		free_string(&buf);
		sqlite3_finalize(res);
		return NULL;
	}
	sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
	sqlite3_bind_text(res, 2, item->guid->ptr, item->guid->len, NULL);
	sqlite3_bind_text(res, 3, item->url->ptr, item->url->len, NULL);
	if (sqlite3_step(res) != SQLITE_ROW) {
		debug_write(DBG_WARN, "could not find that item\n");
		free_string(&buf);
		sqlite3_finalize(res);
		return NULL;
	}
	char *text;
	size_t text_len;
	int pad_height = 0;
	TEXT_TAG_APPEND("Feed: ", 6, ITEM_COLUMN_FEED, config_contents_show_feed)
	TEXT_TAG_APPEND("Title: ", 7, ITEM_COLUMN_TITLE, config_contents_show_title)
	if (config_contents_show_date == true) {
		time_t epoch_time = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_PUBDATE);
		if (epoch_time > 0) {
			struct tm ts = *localtime(&epoch_time);
			char time_str[100];
			if (strftime(time_str, sizeof(time_str), config_contents_date_format, &ts) != 0) {
				cat_string_array(buf, "Date: ", (size_t)6);
				cat_string_array(buf, time_str, strlen(time_str));
				cat_string_char(buf, '\n');
				++pad_height;
			}
		}
	}
	TEXT_TAG_APPEND("Author: ", 8, ITEM_COLUMN_AUTHOR, config_contents_show_author)
	TEXT_TAG_APPEND("Category: ", 10, ITEM_COLUMN_CATEGORY, config_contents_show_category)
	TEXT_TAG_APPEND("Comments: ", 10, ITEM_COLUMN_COMMENTS, config_contents_show_comments)
	TEXT_TAG_APPEND("Link: ", 6, ITEM_COLUMN_URL, config_contents_show_url)
	int32_t not_newline = 0;
	if ((text = (char *)sqlite3_column_text(res, ITEM_COLUMN_CONTENT)) != NULL) {
		cat_string_char(buf, '\n');
		++pad_height;
		text_len = strlen(text);
		if (text_len != 0) {
			cat_string_array(buf, text, text_len);
			for (int i = 0; i < text_len; ++i) {
				if (text[i] == '\n') {
					not_newline = 0;
					++pad_height;
				} else if (not_newline > COLS) {
					not_newline = 0;
					++pad_height;
				} else {
					++not_newline;
				}
			}
		}
	}
	WINDOW *pad = newpad(pad_height, COLS);
	if (pad == NULL) {
		debug_write(DBG_ERROR, "could not create pad window for item contents\n");
		sqlite3_finalize(res);
		free_string(&buf);
		return NULL;
	}
	not_newline = 0;
	char *iter = buf->ptr;
	char *newline_char;
	while (1) {
		newline_char = strchr(iter, '\n');
		if (newline_char != NULL) {
			while (newline_char - iter > COLS) {
				waddnstr(pad, iter, COLS);
				wmove(pad, newlines++, 0);
				iter += COLS;
			}
			waddnstr(pad, iter, newline_char - iter);
			wmove(pad, newlines++, 0);
			iter = newline_char + 1;
		} else {
			while (iter < buf->ptr + buf->len) {
				waddnstr(pad, iter, COLS);
				wmove(pad, newlines++, 0);
				iter += COLS;
			}
			break;
		}
	}
	sqlite3_finalize(res);
	free_string(&buf);
	return pad;
}

static void
scroll_view(int offset)
{
	int new_view_min = view_min + offset;
	int new_view_max = view_max + offset;
	if (new_view_min < 0) {
		new_view_min = 0;
		new_view_max = LINES - 1;
	} else if (new_view_max + 1 >= newlines) {
		new_view_max = newlines - 1;
		new_view_min = new_view_max - LINES;
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
	int new_view_min = new_view_max - LINES;
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
		if      (ch == 'j' || ch == KEY_DOWN)                               { scroll_view(1); }
		else if (ch == 'k' || ch == KEY_UP)                                 { scroll_view(-1); }
		else if (ch == 'h' || ch == KEY_LEFT || ch == config_key_soft_quit) { return MENU_ITEMS; }
		else if (ch == 'g' && input_wgetch() == 'g')                        { scroll_view_top(); }
		else if (ch == 'G')                                                 { scroll_view_bot(); }
		else if (ch == config_key_hard_quit)                                { return MENU_QUIT; }
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
	newlines = 1;
	view_min = 0;
	view_max = LINES - 1;
	window = cat_content(feed_url, item);
	if (window == NULL) {
		status_write("could not obtain contents of item");
		return MENU_CONTENT_ERROR;
	}
	db_update_item_int(feed_url, item, "unread", 0);
	hide_items();
	clear();
	refresh();
	prefresh(window, view_min, 0, 0, 0, LINES - 2, COLS);
	int contents_status = menu_contents();
	delwin(window);
	return contents_status;
}
