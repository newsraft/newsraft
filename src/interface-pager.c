#include <stdlib.h>
#include "newsraft.h"

struct pager_menu {
	WINDOW *window;
	size_t view_min; // index of first visible line (row)
	size_t view_lim; // maximum reachable value of view_min
};

static inline void
write_splitted_wstring_to_window(WINDOW *window, const struct wstring *wbuf)
{
	size_t newlines_count = 0;
	const wchar_t *iter = wbuf->ptr;
	const wchar_t *newline_char = wcschr(iter, L'\n');
	while (newline_char != NULL) {
		wmove(window, newlines_count++, 0);
		waddnwstr(window, iter, newline_char - iter);
		iter = newline_char + 1;
		newline_char = wcschr(iter, L'\n');
	}
	INFO("Number of lines written to the window: %zu.", newlines_count);
}

static bool
update_pager_menu(struct pager_menu *menu, const struct render_block *data_list)
{
	struct wstring *text = render_data(data_list);
	if (text == NULL) {
		FAIL("Failed to render content!");
		fail_status("Can't render content!");
		return false;
	}
	if (text->len == 0) {
		WARN("Rendered content is empty!");
		fail_status("Content is empty!");
		free_wstring(text);
		return false;
	}

	// As long as rendered data was generated according to size of the terminal and
	// every line in it is terminated with newline, we can get required height for
	// pad window by counting newline characters in the rendered text wstring.
	size_t pad_height = 0;
	for (size_t i = 0; i < text->len; ++i) {
		if (text->ptr[i] == L'\n') {
			++pad_height;
		}
	}

	if (menu->window != NULL) {
		delwin(menu->window);
	}

	INFO("Creating pad window with %zu width and %zu height.", list_menu_width, pad_height);
	menu->window = newpad(pad_height, list_menu_width);
	if (menu->window == NULL) {
		FAIL("Failed to create pad window for item contents (newpad returned NULL)!");
		free_wstring(text);
		return false;
	}

	write_splitted_wstring_to_window(menu->window, text);

	menu->view_min = 0;
	menu->view_lim = pad_height > list_menu_height ? pad_height - list_menu_height : 0;

	// We need to call clear and refresh before every prefresh here because
	// after terminal resize (i. e. terminal size increased) in the area where
	// the pad is no longer there, the text of previous buffer may remain.
	clear();
	// Redraw status message since we cleared the whole screen.
	refresh();
	status_update();

	prefresh(menu->window, menu->view_min, 0, 0, 0, list_menu_height - 1, list_menu_width - 1);

	free_wstring(text);

	return true;
}

static void
scroll_view(struct pager_menu *menu, size_t pminrow)
{
	if (pminrow == menu->view_min) {
		return;
	}
	menu->view_min = pminrow;
	prefresh(menu->window, pminrow, 0, 0, 0, list_menu_height - 1, list_menu_width - 1);
}

static inline void
scroll_one_line_down(struct pager_menu *menu)
{
	scroll_view(menu, menu->view_min < menu->view_lim ? menu->view_min + 1 : menu->view_lim);
}

static inline void
scroll_one_line_up(struct pager_menu *menu)
{
	scroll_view(menu, menu->view_min == 0 ? 0 : menu->view_min - 1);
}

static inline void
scroll_one_page_down(struct pager_menu *menu)
{
	scroll_view(menu, menu->view_min + list_menu_height < menu->view_lim ? menu->view_min + list_menu_height : menu->view_lim);
}

static inline void
scroll_one_page_up(struct pager_menu *menu)
{
	scroll_view(menu, menu->view_min > list_menu_height ? menu->view_min - list_menu_height : 0);
}

static inline void
scroll_to_the_beginning(struct pager_menu *menu)
{
	scroll_view(menu, 0);
}

static inline void
scroll_to_the_end(struct pager_menu *menu)
{
	scroll_view(menu, menu->view_lim);
}

// On success - exit by user - returns INPUT_QUIT_SOFT or INPUT_QUIT_HARD.
// On failure returns INPUTS_COUNT.
int
pager_view(const struct render_block *first_block, bool (*custom_input_handler)(void *, input_cmd_id, uint32_t, const struct wstring *), void *data)
{
	pause_list_menu();

	struct pager_menu menu = {NULL, 0, 0};
	if (update_pager_menu(&menu, first_block) == false) {
		// Error message written by update_pager_menu.
		return INPUTS_COUNT;
	}

	input_cmd_id cmd;
	uint32_t count;
	const struct wstring *macro;
	while (true) {
		cmd = get_input_command(&count, &macro);
		if ((cmd == INPUT_SELECT_NEXT) || (cmd == INPUT_ENTER)) {
			scroll_one_line_down(&menu);
		} else if (cmd == INPUT_SELECT_PREV) {
			scroll_one_line_up(&menu);
		} else if (cmd == INPUT_SELECT_NEXT_PAGE) {
			scroll_one_page_down(&menu);
		} else if (cmd == INPUT_SELECT_PREV_PAGE) {
			scroll_one_page_up(&menu);
		} else if (cmd == INPUT_SELECT_FIRST) {
			scroll_to_the_beginning(&menu);
		} else if (cmd == INPUT_SELECT_LAST) {
			scroll_to_the_end(&menu);
		} else if (cmd == INPUT_RESIZE) {
			if (update_pager_menu(&menu, first_block) == false) {
				// Error message written by update_pager_menu.
				return INPUTS_COUNT;
			}
		} else if ((cmd == INPUT_QUIT_SOFT) || (cmd == INPUT_QUIT_HARD)) {
			break;
		} else if (custom_input_handler != NULL) {
			if (custom_input_handler(data, cmd, count, macro) == true) {
				if (update_pager_menu(&menu, first_block) == false) {
					// Error message written by update_pager_menu.
					return INPUTS_COUNT;
				}
			}
		}
	}

	delwin(menu.window);
	resume_list_menu();

	return cmd;
}
