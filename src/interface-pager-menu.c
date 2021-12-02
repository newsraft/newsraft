#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

// Here, a pager is such a thing that serves as a text viewer.

static WINDOW *window;
static int view_min; // index of first visible line (row)
static int pad_height;
static struct string *contents;

static bool
is_wchar_a_breaker(wchar_t wc)
{
	size_t i = 0;
	while (config_break_at[i] != '\0') {
		if (config_break_at[i] == wctob(wc)) {
			return true; // yes
		}
		++i;
	}
	return false; // no
}

static struct wstring *
split_wstring_into_lines(struct wstring *wstr)
{
	struct wstring *broken_wstr = create_wstring(NULL, 0);
	if (broken_wstr == NULL) {
		return NULL; // failure
	}

	wchar_t *line = malloc(sizeof(wchar_t) * (list_menu_width + 1));
	if (line == NULL) {
		free_wstring(broken_wstr);
		return NULL; // failure
	}
	ssize_t line_len = 0;
	ssize_t new_line_len;

	wchar_t *iter = wstr->ptr;
	size_t last_breaker_index_in_line = SIZE_MAX;

	while (*iter != L'\0') {
		if (line_len == list_menu_width) {
			line[line_len] = L'\0';

			if (last_breaker_index_in_line == SIZE_MAX) {
				cat_wstring_array(broken_wstr, line, line_len);
				line_len = 0;
			} else {
				cat_wstring_array(broken_wstr, line, last_breaker_index_in_line + 1);
				new_line_len = 0;
				for (ssize_t i = last_breaker_index_in_line + 1; i < line_len; ++i) {
					line[new_line_len++] = line[i];
				}
				line[new_line_len] = L'\0';
				line_len = new_line_len;
			}

			cat_wstring_wchar(broken_wstr, L'\n');

			last_breaker_index_in_line = SIZE_MAX;
		} else if (*iter == L'\n') {
			cat_wstring_array(broken_wstr, line, line_len);
			cat_wstring_wchar(broken_wstr, L'\n');
			line_len = 0;
			last_breaker_index_in_line = SIZE_MAX;
			++iter;
		} else {
			line[line_len++] = *iter;
			if (is_wchar_a_breaker(*iter)) {
				last_breaker_index_in_line = line_len - 1;
			}
			++iter;
		}
	}

	cat_wstring_array(broken_wstr, line, line_len);
	cat_wstring_wchar(broken_wstr, L'\n');

	free(line);

	return broken_wstr; // success
}

static void
write_broken_wstring_to_pad(WINDOW *pad, const struct wstring *wbuf)
{
	int newlines = 0;
	wchar_t *iter = wbuf->ptr;
	wchar_t *newline_char;
	while (1) {
		newline_char = wcschr(iter, L'\n');
		if (newline_char != NULL) {
			wmove(pad, newlines++, 0);
			waddnwstr(pad, iter, newline_char - iter);
			iter = newline_char + 1;
		} else {
			while (iter < wbuf->ptr + wbuf->len) {
				wmove(pad, newlines++, 0);
				waddnwstr(pad, iter, list_menu_width);
				iter += list_menu_width;
			}
			break;
		}
	}
}

/* static void */
/* write_string_to_pad(WINDOW *pad, const struct string *buf) */
/* { */
/* 	int newlines = 0; */
/* 	char *iter = buf->ptr; */
/* 	char *newline_char; */
/* 	while (1) { */
/* 		newline_char = strchr(iter, '\n'); */
/* 		if (newline_char != NULL) { */
/* 			while (newline_char - iter > list_menu_width) { */
/* 				wmove(pad, newlines++, 0); */
/* 				waddnstr(pad, iter, list_menu_width); */
/* 				iter += list_menu_width; */
/* 			} */
/* 			wmove(pad, newlines++, 0); */
/* 			waddnstr(pad, iter, newline_char - iter); */
/* 			iter = newline_char + 1; */
/* 		} else { */
/* 			while (iter < buf->ptr + buf->len) { */
/* 				wmove(pad, newlines++, 0); */
/* 				waddnstr(pad, iter, list_menu_width); */
/* 				iter += list_menu_width; */
/* 			} */
/* 			break; */
/* 		} */
/* 	} */
/* } */

static WINDOW *
create_window_with_contents(void)
{
	WINDOW *pad;
	if (1) { // buffer is multi-byte (i never seen a feed that is not Unicode in my life)
		struct wstring *wbuf = convert_string_to_wstring(contents);
		if (wbuf == NULL) {
			return NULL;
		}

		// Create string that is splitted with newlines in a blocks of at most N characters,
		// where N is the current width of terminal.
		struct wstring *broken_wbuf = split_wstring_into_lines(wbuf);
		free_wstring(wbuf);
		if (broken_wbuf == NULL) {
			FAIL("Not enough memory for breaking a string into lines!");
			return NULL;
		}

		// As long as we splitted contents with newlines, we can get required height for
		// pad window by counting newline characters in broken_wbuf.
		pad_height = 0;
		for (size_t i = 0; i < broken_wbuf->len; ++i) {
			if (broken_wbuf->ptr[i] == L'\n') ++pad_height;
		}

		pad = newpad(pad_height, list_menu_width);
		if (pad == NULL) {
			FAIL("Could not create pad window for item contents!");
			free_wstring(broken_wbuf);
			return NULL;
		}

		write_broken_wstring_to_pad(pad, broken_wbuf);

		free_wstring(broken_wbuf);
	} else { // buffer is single-byte. again, never happens
		/* pad_height = calculate_pad_height_for_string(contents); */
		/* pad = newpad(pad_height, list_menu_width); */
		/* if (pad == NULL) { */
		/* 	FAIL("Could not create pad window for item contents!"); */
		/* 	return NULL; */
		/* } */

		/* write_string_to_pad(pad, contents); */
	}

	view_min = 0;

	clear();
	refresh();
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
	if (window == NULL) {
		status_write("Can not create window for items contents!");
		return INPUT_QUIT_SOFT; // failure
	}

	set_pager_view_input_handlers();

	int destination;
	do {
		if (window == NULL) {
			status_write("That resize hurts!");
			return INPUT_QUIT_SOFT; // failure
		}
		destination = handle_input();
	} while ((destination != INPUT_QUIT_SOFT) && (destination != INPUT_QUIT_HARD));

	delwin(window);

	return destination; // success
}
