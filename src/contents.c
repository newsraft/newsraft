#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "feedeater.h"
#include "config.h"

static WINDOW *window;
static int view_min;
static int view_max;
static int newlines;

struct {
	const char *name;
	const char *column;
	const int num;
} meta_data[] = {
	{"Feed", "feed", ITEM_COLUMN_FEED},
	{"Title", "title", ITEM_COLUMN_TITLE},
	{"Author", "author", ITEM_COLUMN_AUTHOR},
	{"Category", "category", ITEM_COLUMN_CATEGORY},
	{"Link", "url", ITEM_COLUMN_URL},
	{"Comments", "comments", ITEM_COLUMN_COMMENTS},
};

static int
calculate_pad_height_for_buf(struct string *buf)
{
	int pad_height = 0, not_newline = 0;
	for (size_t i = 0; i < buf->len; ++i) {
		if ((buf->ptr[i] == '\n') || (not_newline > COLS)) {
			not_newline = 0;
			++pad_height;
		} else {
			++not_newline;
		}
	}
	return pad_height;
}

static WINDOW *
cat_content(struct string *feed_url, struct item_entry *item_data)
{
	struct string *buf = create_string();
	if (buf == NULL) {
		debug_write(DBG_ERR, "can't create buffer for content of item\n");
		return NULL;
	}

	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(db, "SELECT * FROM items WHERE feed = ? AND guid = ? AND url = ? LIMIT 1", -1, &res, 0);
	if (rc != SQLITE_OK) {
		debug_write(DBG_WARN, "failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize(res);
		free_string(buf);
		return NULL;
	}
	sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
	sqlite3_bind_text(res, 2, item_data->guid->ptr, item_data->guid->len, NULL);
	sqlite3_bind_text(res, 3, item_data->url->ptr, item_data->url->len, NULL);
	if (sqlite3_step(res) != SQLITE_ROW) {
		debug_write(DBG_WARN, "could not find that item\n");
		sqlite3_finalize(res);
		free_string(buf);
		return NULL;
	}

	char *draft_meta_data_order = malloc(sizeof(char) * (strlen(config_contents_meta_data) + 1));
	if (draft_meta_data_order == NULL) {
		debug_write(DBG_ERR, "not enough memory for tokenizing order of meta data tags\n");
		free_string(buf);
		sqlite3_finalize(res);
		return NULL;
	}
	strcpy(draft_meta_data_order, config_contents_meta_data);
	char *text, *saveptr = NULL;
	size_t text_len;
	char *meta_data_entry = strtok_r(draft_meta_data_order, ",", &saveptr);
	do {
		if (strcmp(meta_data_entry, "date") == 0) {
			time_t epoch_time = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_PUBDATE);
			if (epoch_time > 0) {
				struct tm ts = *localtime(&epoch_time);
				char time_str[100];
				if (strftime(time_str, sizeof(time_str), config_contents_date_format, &ts) != 0) {
					cat_string_array(buf, "Date: ", (size_t)6);
					cat_string_array(buf, time_str, strlen(time_str));
					cat_string_char(buf, '\n');
				}
			}
			continue;
		}
		text = NULL;
		for (size_t i = 0; i < LENGTH(meta_data); ++i) {
			if (strcmp(meta_data_entry, meta_data[i].column) == 0) {
				text = (char *)sqlite3_column_text(res, meta_data[i].num);
				if (text == NULL) break;
				text_len = strlen(text);
				if (text_len != 0) {
					cat_string_array(buf, (char *)meta_data[i].name, strlen(meta_data[i].name));
					cat_string_array(buf, ": ", (size_t)2);
					cat_string_array(buf, text, text_len);
					cat_string_char(buf, '\n');
				}
				break;
			}
		}
	} while ((meta_data_entry = strtok_r(NULL, ",", &saveptr)) != NULL);
	free(draft_meta_data_order);

	text = (char *) sqlite3_column_text(res, ITEM_COLUMN_CONTENT);
	if (text != NULL) {
		char *plain_text = plainify_html(text, strlen(text));
		if (plain_text != NULL) {
			cat_string_char(buf, '\n');
			text_len = strlen(plain_text);
			if (text_len != 0) {
				cat_string_array(buf, plain_text, text_len);
			}
			free(plain_text);
		}
	}

	//int pad_height = 1;
	int pad_height = calculate_pad_height_for_buf(buf);
	WINDOW *pad = newpad(pad_height, COLS);
	if (pad == NULL) {
		debug_write(DBG_ERR, "could not create pad window for item contents\n");
		sqlite3_finalize(res);
		free_string(buf);
		return NULL;
	}

	if (1) { // buffer is multi-byte
		size_t mbslen = mbstowcs(NULL, buf->ptr, 0);
		if (mbslen == (size_t)-1) {
			delwin(pad);
			sqlite3_finalize(res);
			free_string(buf);
			return NULL;
		}
		wchar_t *wcs = calloc(mbslen + 1, sizeof(wchar_t));
		if (wcs == NULL) {
			delwin(pad);
			sqlite3_finalize(res);
			free_string(buf);
			return NULL;
		}
		if (mbstowcs(wcs, buf->ptr, mbslen + 1) == (size_t)-1) {
			free(wcs);
			delwin(pad);
			sqlite3_finalize(res);
			free_string(buf);
			return NULL;
		}
		wchar_t *iter = wcs;
		wchar_t *newline_char;
		while (1) {
			newline_char = wcschr(iter, L'\n');
			if (newline_char != NULL) {
				while (newline_char - iter > COLS) {
					waddnwstr(pad, iter, COLS);
					//wresize(pad, ++pad_height, COLS);
					wmove(pad, newlines++, 0);
					iter += COLS;
				}
				waddnwstr(pad, iter, newline_char - iter);
				//wresize(pad, ++pad_height, COLS);
				wmove(pad, newlines++, 0);
				iter = newline_char + 1;
			} else {
				while (iter < wcs + mbslen) {
					waddnwstr(pad, iter, COLS);
					//wresize(pad, ++pad_height, COLS);
					wmove(pad, newlines++, 0);
					iter += COLS;
				}
				break;
			}
		}
		free(wcs);
	} else { // buffer is single-byte
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
	}

	sqlite3_finalize(res);
	free_string(buf);
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
		new_view_min = new_view_max - LINES + 1;
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
	int new_view_min = new_view_max - LINES + 1;
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
		if      (ch == 'j' || ch == KEY_DOWN)                            { scroll_view(1); }
		else if (ch == 'k' || ch == KEY_UP)                              { scroll_view(-1); }
		else if (ch == KEY_NPAGE)                                        { scroll_view(LINES - 1); }
		else if (ch == KEY_PPAGE)                                        { scroll_view(-LINES + 1); }
		else if (ch == 'G' || ch == KEY_END)                             { scroll_view_bot(); }
		else if ((ch == 'g' && input_wgetch() == 'g') || ch == KEY_HOME) { scroll_view_top(); }
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
		else if (ch == 'h' || ch == KEY_LEFT || ch == config_key_soft_quit) { return MENU_ITEMS; }
		else if (ch == config_key_hard_quit)                                { return MENU_QUIT; }
	}
}

int
contents_menu(struct item_line *item)
{
	newlines = 1;
	view_min = 0;
	view_max = LINES - 1;
	window = cat_content(item->feed_url, item->data);
	if (window == NULL) {
		status_write("could not obtain contents of item");
		return MENU_CONTENT_ERROR;
	}
	db_update_item_int(item->feed_url, item->data, "unread", 0);
	hide_items();
	clear();
	refresh();
	prefresh(window, view_min, 0, 0, 0, LINES - 2, COLS);
	int contents_status = menu_contents();
	delwin(window);
	return contents_status;
}
