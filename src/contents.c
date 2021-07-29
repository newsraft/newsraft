#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "feedeater.h"
#include "config.h"

static WINDOW *window;
static int view_min; // index of first visible line (row)
static int view_area_height;
static int pad_height;

struct {
	const char *name;
	const char *column; /* name of column in database table */
	const int num;
} meta_data[] = {
	{"Feed", "feed", ITEM_COLUMN_FEED},
	{"Title", "title", ITEM_COLUMN_TITLE},
	{"Author", "author", ITEM_COLUMN_AUTHOR},
	{"Category", "category", ITEM_COLUMN_CATEGORY},
	{"Link", "url", ITEM_COLUMN_URL},
	{"Comments", "comments", ITEM_COLUMN_COMMENTS},
};

static sqlite3_stmt *
find_item_by_its_data_in_db(struct string *feed_url, struct item_entry *item_data)
{
	sqlite3_stmt *res;
	int rc = sqlite3_prepare_v2(db, "SELECT * FROM items WHERE feed = ? AND guid = ? AND url = ? AND title = ? LIMIT 1", -1, &res, 0);
	if (rc != SQLITE_OK) {
		debug_write(DBG_WARN, "failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
		return NULL; // fail
	}
	sqlite3_bind_text(res, 1, feed_url->ptr, feed_url->len, NULL);
	sqlite3_bind_text(res, 2, item_data->guid->ptr, item_data->guid->len, NULL);
	sqlite3_bind_text(res, 3, item_data->url->ptr, item_data->url->len, NULL);
	sqlite3_bind_text(res, 4, item_data->title->ptr, item_data->title->len, NULL);
	if (sqlite3_step(res) != SQLITE_ROW) {
		debug_write(DBG_WARN, "could not find that item\n");
		sqlite3_finalize(res);
		return NULL; // fail
	}
	return res; // success
}

static void
cat_item_date_to_buf(sqlite3_stmt *res, struct string *buf)
{
	time_t item_date = 0,
	       item_pubdate = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_PUBDATE),
	       item_upddate = (time_t)sqlite3_column_int64(res, ITEM_COLUMN_UPDDATE);
	item_date = item_pubdate > item_upddate ? item_pubdate : item_upddate;
	if (item_date == 0) {
		return;
	}
	struct string *date_str = get_config_date_str(&item_date);
	if (date_str == NULL) {
		return;
	}
	cat_string_array(buf, "Date: ", (size_t)6);
	cat_string_string(buf, date_str);
	cat_string_char(buf, '\n');
	free_string(date_str);
}

static void
cat_item_meta_data_to_buf(sqlite3_stmt *res, struct string *buf, int meta_data_index)
{
	char *text = (char *)sqlite3_column_text(res, meta_data[meta_data_index].num);
	if (text == NULL) {
		return;
	}
	size_t text_len = strlen(text);
	if (text_len == 0) {
		return;
	}
	cat_string_array(buf,
	                 (char *)meta_data[meta_data_index].name,
	                 /* strlen is optimized by compiler, right? */
	                 strlen(meta_data[meta_data_index].name));
	cat_string_array(buf, ": ", (size_t)2);
	cat_string_array(buf, text, text_len);
	cat_string_char(buf, '\n');
}

static struct string *
get_contents_of_item(sqlite3_stmt *res)
{
	struct string *buf = create_empty_string();
	if (buf == NULL) {
		debug_write(DBG_ERR, "can't create buffer for content of item\n");
		return NULL;
	}
	char *restrict draft_meta_data_order = malloc(sizeof(char) * (strlen(config_contents_meta_data) + 1));
	if (draft_meta_data_order == NULL) {
		free_string(buf);
		debug_write(DBG_ERR, "not enough memory for tokenizing order of meta data tags\n");
		return NULL;
	}
	strcpy(draft_meta_data_order, config_contents_meta_data);
	char *saveptr = NULL,
	     *meta_data_entry = strtok_r(draft_meta_data_order, ",", &saveptr);
	do {
		if (strcmp(meta_data_entry, "date") == 0) {
			cat_item_date_to_buf(res, buf);
			continue;
		}
		for (size_t i = 0; i < LENGTH(meta_data); ++i) {
			if (strcmp(meta_data_entry, meta_data[i].column) == 0) {
				cat_item_meta_data_to_buf(res, buf, i);
				break;
			}
		}
	} while ((meta_data_entry = strtok_r(NULL, ",", &saveptr)) != NULL);
	free(draft_meta_data_order);

	char *text = (char *)sqlite3_column_text(res, ITEM_COLUMN_CONTENT);
	if (text != NULL) {
		size_t text_len = strlen(text);
		if (text_len != 0) {
			struct string *plain_text = plainify_html(text, text_len);
			if (plain_text != NULL) {
				if (plain_text->len != 0) {
					cat_string_char(buf, '\n');
					value_strip_whitespace(plain_text->ptr, &plain_text->len);
					cat_string_string(buf, plain_text);
				}
				free_string(plain_text);
			}
		}
	}
	return buf;
}

static int
calculate_pad_height_for_wcs(wchar_t *buf, size_t len)
{
	int pad_height = 1, not_newline = 0;
	for (size_t i = 0; i < len; ++i) {
		if ((buf[i] == L'\n') || (not_newline > COLS)) {
			not_newline = 0;
			++pad_height;
		} else {
			++not_newline;
		}
	}
	return pad_height;
}

static int
calculate_pad_height_for_buf(struct string *buf)
{
	int pad_height = 1, not_newline = 0;
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
create_contents_window(struct string *buf)
{
	WINDOW *pad;
	int newlines = 0;
	if (1) { // buffer is multi-byte
		size_t mbslen = mbstowcs(NULL, buf->ptr, 0);
		if (mbslen == (size_t)-1) {
			return NULL;
		}
		wchar_t *wcs = calloc(mbslen + 1, sizeof(wchar_t));
		if (wcs == NULL) {
			return NULL;
		}
		if (mbstowcs(wcs, buf->ptr, mbslen + 1) == (size_t)-1) {
			free(wcs);
			return NULL;
		}

		pad_height = calculate_pad_height_for_wcs(wcs, mbslen);
		pad = newpad(pad_height, COLS);
		if (pad == NULL) {
			debug_write(DBG_ERR, "could not create pad window for item contents\n");
			free(wcs);
			return NULL;
		}

		wchar_t *iter = wcs;
		wchar_t *newline_char;
		while (1) {
			newline_char = wcschr(iter, L'\n');
			if (newline_char != NULL) {
				while (newline_char - iter > COLS) {
					wmove(pad, newlines++, 0);
					waddnwstr(pad, iter, COLS);
					iter += COLS;
				}
				wmove(pad, newlines++, 0);
				waddnwstr(pad, iter, newline_char - iter);
				iter = newline_char + 1;
			} else {
				while (iter < wcs + mbslen) {
					wmove(pad, newlines++, 0);
					waddnwstr(pad, iter, COLS);
					iter += COLS;
				}
				break;
			}
		}
		debug_write(pad_height == newlines ? DBG_INFO : DBG_ERR,
		            "created pad of %d lines height, wrote %d lines to this pad\n",
		            pad_height, newlines);
		free(wcs);
	} else { // buffer is single-byte
		pad_height = calculate_pad_height_for_buf(buf);
		pad = newpad(pad_height, COLS);
		if (pad == NULL) {
			debug_write(DBG_ERR, "could not create pad window for item contents\n");
			return NULL;
		}
		debug_write(DBG_INFO, "created pad of %d lines height\n", pad_height);

		char *iter = buf->ptr;
		char *newline_char;
		while (1) {
			newline_char = strchr(iter, '\n');
			if (newline_char != NULL) {
				while (newline_char - iter > COLS) {
					wmove(pad, newlines++, 0);
					waddnstr(pad, iter, COLS);
					iter += COLS;
				}
				wmove(pad, newlines++, 0);
				waddnstr(pad, iter, newline_char - iter);
				iter = newline_char + 1;
			} else {
				while (iter < buf->ptr + buf->len) {
					wmove(pad, newlines++, 0);
					waddnstr(pad, iter, COLS);
					iter += COLS;
				}
				break;
			}
		}
	}

	return pad;
}

static void
scroll_view(int offset)
{
	int new_view_min = view_min + offset;
	if (new_view_min < 0) {
		new_view_min = 0;
	} else if (new_view_min + view_area_height >= pad_height) {
		new_view_min = pad_height - view_area_height;
	}
	if (new_view_min != view_min) {
		view_min = new_view_min;
		prefresh(window, view_min, 0, 0, 0, view_area_height - 1, COLS - 1);
	}
}

static void
scroll_view_top(void)
{
	int new_view_min = 0;
	if (new_view_min != view_min) {
		view_min = new_view_min;
		prefresh(window, view_min, 0, 0, 0, view_area_height - 1, COLS - 1);
	}
}

static void
scroll_view_bot(void)
{
	int new_view_min = pad_height - view_area_height;
	if (new_view_min != view_min) {
		view_min = new_view_min;
		prefresh(window, view_min, 0, 0, 0, view_area_height - 1 , COLS - 1);
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
		else if (ch == KEY_NPAGE)                                        { scroll_view(view_area_height); }
		else if (ch == KEY_PPAGE)                                        { scroll_view(-view_area_height); }
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
	debug_write(DBG_INFO, "trying to view an \"%s\" item of \"%s\" feed\n", item->data->title->ptr, item->feed_url->ptr);
	pad_height = 0;
	view_area_height = LINES - 1;
	view_min = 0;
	sqlite3_stmt *res = find_item_by_its_data_in_db(item->feed_url, item->data);
	if (res == NULL) {
		return MENU_CONTENT_ERROR;
	}
	struct string *buf = get_contents_of_item(res);
	if (buf == NULL) {
		sqlite3_finalize(res);
		return MENU_CONTENT_ERROR;
	}
	window = create_contents_window(buf);
	free_string(buf);
	sqlite3_finalize(res);
	if (window == NULL) {
		status_write("could not obtain contents of item");
		return MENU_CONTENT_ERROR;
	}
	db_update_item_int(item->feed_url, item->data, "unread", 0);
	hide_items();
	clear();
	refresh();
	prefresh(window, view_min, 0, 0, 0, view_area_height - 1, COLS - 1);
	int contents_status = menu_contents();
	delwin(window);
	return contents_status;
}
