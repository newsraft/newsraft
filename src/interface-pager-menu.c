#include <string.h>
#include "feedeater.h"

// Here, a pager is such a thing that serves as a text viewer.

static WINDOW *window;
static int view_min; // index of first visible line (row)
static int pad_height;
static struct string *contents;

static int
calculate_pad_height_for_wstring(const struct wstring *wstr)
{
	int pad_height = 1, not_newline = 0;
	for (size_t i = 0; i < wstr->len; ++i) {
		if ((wstr->ptr[i] == L'\n') || (not_newline > list_menu_width)) {
			not_newline = 0;
			++pad_height;
		} else {
			++not_newline;
		}
	}
	return pad_height;
}

static int
calculate_pad_height_for_string(const struct string *buf)
{
	int pad_height = 1, not_newline = 0;
	for (size_t i = 0; i < buf->len; ++i) {
		if ((buf->ptr[i] == '\n') || (not_newline > list_menu_width)) {
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
static void
make_sure_pad_has_enough_lines(WINDOW *pad, int needed_lines)
{
	if (needed_lines > pad_height) {
		WARN("Due to unfixed bug, pad had to expand from %d to %d lines!", pad_height, needed_lines);
		pad_height = needed_lines;
		wresize(pad, pad_height, list_menu_width);
	}
}

static void
write_wstring_to_pad_without_linebreaks(WINDOW *pad, const struct wstring *wbuf)
{
	int newlines = 0;
	wchar_t *iter = wbuf->ptr;
	wchar_t *newline_char;
	while (1) {
		newline_char = wcschr(iter, L'\n');
		if (newline_char != NULL) {
			while (newline_char - iter > list_menu_width) {
				make_sure_pad_has_enough_lines(pad, newlines + 1);
				wmove(pad, newlines++, 0);
				waddnwstr(pad, iter, list_menu_width);
				iter += list_menu_width;
			}
			make_sure_pad_has_enough_lines(pad, newlines + 1);
			wmove(pad, newlines++, 0);
			waddnwstr(pad, iter, newline_char - iter);
			iter = newline_char + 1;
		} else {
			while (iter < wbuf->ptr + wbuf->len) {
				make_sure_pad_has_enough_lines(pad, newlines + 1);
				wmove(pad, newlines++, 0);
				waddnwstr(pad, iter, list_menu_width);
				iter += list_menu_width;
			}
			break;
		}
	}
}

static void
write_string_to_pad_without_linebreaks(WINDOW *pad, const struct string *buf)
{
	int newlines = 0;
	char *iter = buf->ptr;
	char *newline_char;
	while (1) {
		newline_char = strchr(iter, '\n');
		if (newline_char != NULL) {
			while (newline_char - iter > list_menu_width) {
				make_sure_pad_has_enough_lines(pad, newlines + 1);
				wmove(pad, newlines++, 0);
				waddnstr(pad, iter, list_menu_width);
				iter += list_menu_width;
			}
			make_sure_pad_has_enough_lines(pad, newlines + 1);
			wmove(pad, newlines++, 0);
			waddnstr(pad, iter, newline_char - iter);
			iter = newline_char + 1;
		} else {
			while (iter < buf->ptr + buf->len) {
				make_sure_pad_has_enough_lines(pad, newlines + 1);
				wmove(pad, newlines++, 0);
				waddnstr(pad, iter, list_menu_width);
				iter += list_menu_width;
			}
			break;
		}
	}
}

static WINDOW *
create_window_with_contents(void)
{
	WINDOW *pad;
	if (1) { // buffer is multi-byte
		struct wstring *wbuf = convert_string_to_wstring(contents);
		if (wbuf == NULL) {
			return NULL;
		}

		pad_height = calculate_pad_height_for_wstring(wbuf);
		pad = newpad(pad_height, list_menu_width);
		if (pad == NULL) {
			FAIL("Could not create pad window for item contents!");
			free_wstring(wbuf);
			return NULL;
		}

		write_wstring_to_pad_without_linebreaks(pad, wbuf);

		free_wstring(wbuf);
	} else { // buffer is single-byte
		pad_height = calculate_pad_height_for_string(contents);
		pad = newpad(pad_height, list_menu_width);
		if (pad == NULL) {
			FAIL("Could not create pad window for item contents!");
			return NULL;
		}

		write_string_to_pad_without_linebreaks(pad, contents);
	}
	clear();
	refresh();
	view_min = 0;
	prefresh(pad, view_min, 0, 0, 0, list_menu_height - 1, list_menu_width - 1);
	return pad;
}

static void
scroll_view(int offset)
{
	int new_view_min = view_min + offset;
	if (new_view_min < 0) {
		new_view_min = 0;
	} else if (new_view_min + list_menu_height >= pad_height) {
		new_view_min = pad_height - list_menu_height;
	}
	if (new_view_min != view_min) {
		view_min = new_view_min;
		prefresh(window, view_min, 0, 0, 0, list_menu_height - 1, list_menu_width - 1);
	}
}

static void
scroll_view_top(void)
{
	int new_view_min = 0;
	if (new_view_min != view_min) {
		view_min = new_view_min;
		prefresh(window, view_min, 0, 0, 0, list_menu_height - 1, list_menu_width - 1);
	}
}

static void
scroll_view_bot(void)
{
	int new_view_min = pad_height - list_menu_height;
	if (new_view_min != view_min) {
		view_min = new_view_min;
		prefresh(window, view_min, 0, 0, 0, list_menu_height - 1 , list_menu_width - 1);
	}
}

static void
scroll_down_a_line(void){
	scroll_view(1);
}

static void
scroll_down_a_page(void){
	scroll_view(list_menu_height);
}

static void
scroll_up_a_line(void){
	scroll_view(-1);
}

static void
scroll_up_a_page(void){
	scroll_view(-list_menu_height);
}

static void
redraw_content_by_resize(void)
{
	delwin(window);
	window = create_window_with_contents();
}

static void
set_pager_view_input_handlers(void)
{
	reset_input_handlers();
	set_input_handler(INPUT_SELECT_NEXT, &scroll_down_a_line);
	set_input_handler(INPUT_SELECT_NEXT_PAGE, &scroll_down_a_page);
	set_input_handler(INPUT_SELECT_PREV, &scroll_up_a_line);
	set_input_handler(INPUT_SELECT_PREV_PAGE, &scroll_up_a_page);
	set_input_handler(INPUT_SELECT_FIRST, &scroll_view_top);
	set_input_handler(INPUT_SELECT_LAST, &scroll_view_bot);
	set_input_handler(INPUT_RESIZE, &redraw_content_by_resize);
}

int
pager_view(struct string *data)
{
	contents = data;

	window = create_window_with_contents();

	set_pager_view_input_handlers();

	int destination;
	do {
		destination = handle_input();
	} while ((destination != INPUT_QUIT_SOFT) && (destination != INPUT_QUIT_HARD));

	delwin(window);

	return destination;
}
