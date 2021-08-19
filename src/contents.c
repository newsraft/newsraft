#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "feedeater.h"

static WINDOW *window;
static int view_min; // index of first visible line (row)
static int view_area_height;
static int pad_height;

static sqlite3_stmt *
find_item_by_its_rowid_in_db(int rowid)
{
	sqlite3_stmt *res;
	if (sqlite3_prepare_v2(db, "SELECT * FROM items WHERE rowid = ? LIMIT 1", -1, &res, 0) != SQLITE_OK) {
		DEBUG_WRITE_DB_PREPARE_FAIL;
		return NULL; // fail
	}
	sqlite3_bind_int(res, 1, rowid);
	if (sqlite3_step(res) != SQLITE_ROW) {
		debug_write(DBG_WARN, "could not find that item\n");
		sqlite3_finalize(res);
		return NULL; // fail
	}
	return res; // success
}

static struct string *
get_contents_of_item(sqlite3_stmt *res)
{
	struct string *buf = create_empty_string();
	if (buf == NULL) {
		debug_write(DBG_ERR, "can't create buffer for content of item\n");
		return NULL;
	}

	if (cat_item_meta_data_to_buf(buf, res) != 0) {
		free_string(buf);
		return NULL;
	}

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
calculate_pad_height_for_wstring(struct wstring *wstr)
{
	int pad_height = 1, not_newline = 0;
	for (size_t i = 0; i < wstr->len; ++i) {
		if ((wstr->ptr[i] == L'\n') || (not_newline > COLS)) {
			not_newline = 0;
			++pad_height;
		} else {
			++not_newline;
		}
	}
	return pad_height;
}

static int
calculate_pad_height_for_string(struct string *buf)
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

/* in some cases calculate_pad_height_for_wstring and calculate_pad_height_for_string
 * functions are returning wrong number of lines needed for pad to store all data of buf.
 * TODO >> fix these two functions above, please << TODO
 * but for now use this function to resize pad everytime there is so much data in buffer. */
void
make_sure_pad_has_enough_lines(WINDOW *pad, int needed_lines)
{
	if (needed_lines > pad_height) {
		debug_write(DBG_WARN, "pad had to expand from %d to %d lines!\n", pad_height, needed_lines);
		pad_height = needed_lines;
		wresize(pad, pad_height, COLS);
	}
}

static void
write_wstring_to_pad_without_linebreaks(WINDOW *pad, struct wstring *wbuf)
{
	int newlines = 0;
	wchar_t *iter = wbuf->ptr;
	wchar_t *newline_char;
	while (1) {
		newline_char = wcschr(iter, L'\n');
		if (newline_char != NULL) {
			while (newline_char - iter > COLS) {
				make_sure_pad_has_enough_lines(pad, newlines + 1);
				wmove(pad, newlines++, 0);
				waddnwstr(pad, iter, COLS);
				iter += COLS;
			}
			make_sure_pad_has_enough_lines(pad, newlines + 1);
			wmove(pad, newlines++, 0);
			waddnwstr(pad, iter, newline_char - iter);
			iter = newline_char + 1;
		} else {
			while (iter < wbuf->ptr + wbuf->len) {
				make_sure_pad_has_enough_lines(pad, newlines + 1);
				wmove(pad, newlines++, 0);
				waddnwstr(pad, iter, COLS);
				iter += COLS;
			}
			break;
		}
	}
}

static void
write_string_to_pad_without_linebreaks(WINDOW *pad, struct string *buf)
{
	int newlines = 0;
	char *iter = buf->ptr;
	char *newline_char;
	while (1) {
		newline_char = strchr(iter, '\n');
		if (newline_char != NULL) {
			while (newline_char - iter > COLS) {
				make_sure_pad_has_enough_lines(pad, newlines + 1);
				wmove(pad, newlines++, 0);
				waddnstr(pad, iter, COLS);
				iter += COLS;
			}
			make_sure_pad_has_enough_lines(pad, newlines + 1);
			wmove(pad, newlines++, 0);
			waddnstr(pad, iter, newline_char - iter);
			iter = newline_char + 1;
		} else {
			while (iter < buf->ptr + buf->len) {
				make_sure_pad_has_enough_lines(pad, newlines + 1);
				wmove(pad, newlines++, 0);
				waddnstr(pad, iter, COLS);
				iter += COLS;
			}
			break;
		}
	}
}

static WINDOW *
create_contents_window(struct string *buf)
{
	WINDOW *pad;
	if (1) { // buffer is multi-byte
		struct wstring *wbuf = convert_string_to_wstring(buf);
		if (wbuf == NULL) {
			return NULL;
		}

		pad_height = calculate_pad_height_for_wstring(wbuf);
		pad = newpad(pad_height, COLS);
		if (pad == NULL) {
			debug_write(DBG_ERR, "could not create pad window for item contents\n");
			free_wstring(wbuf);
			return NULL;
		}

		write_wstring_to_pad_without_linebreaks(pad, wbuf);

		free_wstring(wbuf);
	} else { // buffer is single-byte
		pad_height = calculate_pad_height_for_string(buf);
		pad = newpad(pad_height, COLS);
		if (pad == NULL) {
			debug_write(DBG_ERR, "could not create pad window for item contents\n");
			return NULL;
		}

		write_string_to_pad_without_linebreaks(pad, buf);
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

static void
view_select_next(void){
	scroll_view(1);
}

static void
view_select_prev(void){
	scroll_view(-1);
}

static void
redraw_content_by_resize(void)
{
	view_area_height = LINES - 1;
	prefresh(window, view_min, 0, 0, 0, view_area_height - 1, COLS - 1);
}

static void
set_content_input_handlers(void)
{
	reset_input_handlers();
	set_input_handler(INPUT_SELECT_NEXT, &view_select_next);
	set_input_handler(INPUT_SELECT_PREV, &view_select_prev);
	set_input_handler(INPUT_SELECT_FIRST, &scroll_view_top);
	set_input_handler(INPUT_SELECT_LAST, &scroll_view_bot);
	set_input_handler(INPUT_RESIZE, &redraw_content_by_resize);
}

int
enter_item_contents_menu_loop(int rowid)
{
	pad_height = 0;
	view_area_height = LINES - 1;
	view_min = 0;
	sqlite3_stmt *res = find_item_by_its_rowid_in_db(rowid);
	if (res == NULL) {
		return INPUTS_COUNT; // error
	}
	struct string *buf = get_contents_of_item(res);
	sqlite3_finalize(res);
	if (buf == NULL) {
		return INPUTS_COUNT; // error
	}
	window = create_contents_window(buf);
	free_string(buf);
	if (window == NULL) {
		status_write("could not obtain contents of item");
		return INPUTS_COUNT; // error
	}
	db_update_item_int(rowid, "unread", 0);
	clear();
	refresh();
	prefresh(window, view_min, 0, 0, 0, view_area_height - 1, COLS - 1);
	int dest;
	set_content_input_handlers();
	while ((dest = handle_input()) == INPUT_ENTER);
	delwin(window);
	return dest;
}
