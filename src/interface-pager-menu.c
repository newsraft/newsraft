#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

// Here, a pager is such a thing that serves as a text viewer.

const struct content_list *data_list;
static WINDOW *window;
static int view_min; // index of first visible line (row)
static int pad_height;

static inline void
write_splitted_wstring_to_pad(WINDOW *pad, const struct wstring *wbuf)
{
	int newlines = 0;
	const wchar_t *iter = wbuf->ptr;
	const wchar_t *newline_char = wcschr(iter, L'\n');
	while (newline_char != NULL) {
		wmove(pad, newlines++, 0);
		waddnwstr(pad, iter, newline_char - iter);
		iter = newline_char + 1;
		newline_char = wcschr(iter, L'\n');
	}
	INFO("Wrote %d lines to pad window.", newlines);
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
		if (text->ptr[i] == L'\n') ++pad_height;
	}

	INFO("Creating pad window with %d height and %d width.", pad_height, list_menu_width);
	WINDOW *pad = newpad(pad_height, list_menu_width);
	if (pad == NULL) {
		FAIL("Failed to create pad window for item contents (newpad returned NULL)!");
		free_wstring(text);
		return NULL;
	}

	write_splitted_wstring_to_pad(pad, text);

	view_min = 0;

	clear();
	refresh();
	prefresh(pad, view_min, 0, 0, 0, list_menu_height - 1, list_menu_width - 1);

	free_wstring(text);

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

// On success - exit by user - returns INPUT_QUIT_SOFT or INPUT_QUIT_HARD.
// On failure returns INPUTS_COUNT.
int
pager_view(const struct content_list *data_list_arg)
{
	data_list = data_list_arg;

	window = create_window_with_contents();
	if (window == NULL) {
		// Error message is written by create_window_with_contents
		return INPUTS_COUNT;
	}

	set_pager_view_input_handlers();

	int destination;
	do {
		destination = handle_input();
	} while ((destination != INPUT_QUIT_SOFT) && (destination != INPUT_QUIT_HARD));

	if (window != NULL) {
		// It may become a NULL if some crazy resize happened.
		delwin(window);
	}

	return destination;
}
