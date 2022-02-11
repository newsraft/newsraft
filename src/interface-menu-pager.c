#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

// Here, a pager is such a thing that serves as a text viewer.

const struct render_block *data_list;
static WINDOW *window;
static size_t view_min; // index of first visible line (row)
static size_t view_lim; // maximum reachable value of view_min
static size_t pad_height;

static inline void
write_splitted_wstring_to_pad(WINDOW *pad, const struct wstring *wbuf)
{
	size_t newlines = 0;
	const wchar_t *iter = wbuf->ptr;
	const wchar_t *newline_char = wcschr(iter, L'\n');
	while (newline_char != NULL) {
		wmove(pad, newlines++, 0);
		waddnwstr(pad, iter, newline_char - iter);
		iter = newline_char + 1;
		newline_char = wcschr(iter, L'\n');
	}
	INFO("Wrote %zu lines to pad window.", newlines);
}

static WINDOW *
create_window_with_contents(void)
{
	struct wstring *text = render_data(data_list);
	if (text == NULL) {
		status_write("Can not create window for items contents!");
		FAIL("Render of item data failed!\n");
		return NULL;
	}
	if (text->len == 0) {
		status_write("Item contents is empty!");
		WARN("Item contents is empty!");
		free_wstring(text);
		return NULL;
	}

	// As long as rendered data was generated according to size of the terminal and
	// every line in it is terminated with newline, we can get required height for
	// pad window by counting newline characters in the rendered text wstring.
	pad_height = 0;
	for (size_t i = 0; i < text->len; ++i) {
		if (text->ptr[i] == L'\n') {
			++pad_height;
		}
	}

	INFO("Creating pad window with %zu width and %zu height.", list_menu_width, pad_height);
	WINDOW *pad = newpad(pad_height, list_menu_width);
	if (pad == NULL) {
		FAIL("Failed to create pad window for item contents (newpad returned NULL)!");
		free_wstring(text);
		return NULL;
	}

	write_splitted_wstring_to_pad(pad, text);

	view_min = 0;
	view_lim = pad_height > list_menu_height ? pad_height - list_menu_height : 0;

	clear();
	refresh();
	status_update();
	prefresh(pad, view_min, 0, 0, 0, list_menu_height - 1, list_menu_width - 1);

	free_wstring(text);

	return pad;
}

static void
scroll_view(size_t pminrow)
{
	if (pminrow == view_min) {
		return;
	}
	view_min = pminrow;
	prefresh(window, pminrow, 0, 0, 0, list_menu_height - 1, list_menu_width - 1);
}

// On success - exit by user - returns INPUT_QUIT_SOFT or INPUT_QUIT_HARD.
// On failure returns INPUTS_COUNT.
int
pager_view(const struct render_block *first_block)
{
	data_list = first_block;

	window = create_window_with_contents();
	if (window == NULL) {
		// Error message is written by create_window_with_contents
		return INPUTS_COUNT;
	}

	status_clean();

	input_cmd_id cmd;
	while (true) {
		cmd = get_input_command();
		if (cmd == INPUT_SELECT_NEXT) {
			scroll_view(view_min < view_lim ? view_min + 1 : view_lim);
		} else if (cmd == INPUT_SELECT_PREV) {
			scroll_view(view_min == 0 ? 0 : view_min - 1);
		} else if (cmd == INPUT_SELECT_NEXT_PAGE) {
			scroll_view(view_min + list_menu_height < view_lim ? view_min + list_menu_height : view_lim);
		} else if (cmd == INPUT_SELECT_PREV_PAGE) {
			scroll_view(view_min > list_menu_height ? view_min - list_menu_height : 0);
		} else if (cmd == INPUT_SELECT_FIRST) {
			scroll_view(0);
		} else if (cmd == INPUT_SELECT_LAST) {
			scroll_view(view_lim);
		} else if (cmd == INPUT_RESIZE) {
			delwin(window);
			window = create_window_with_contents();
		} else if ((cmd == INPUT_QUIT_SOFT) || (cmd == INPUT_QUIT_HARD)) {
			break;
		}
	}

	delwin(window);

	return cmd;
}
