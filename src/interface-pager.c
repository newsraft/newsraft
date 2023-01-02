#include <curses.h>
#include "newsraft.h"

static WINDOW *pager_window = NULL;
static size_t view_min = 0; // Index of first visible line (row).
static size_t view_lim = 0; // Maximum reachable value of view_min.
static struct render_blocks_list *blocks;

static inline void
write_splitted_wstring_to_window(const struct wstring *wstr)
{
	size_t vertical_pos = 0, horizontal_pos = 0, hint_index = 0;
	for (size_t i = 0; i < wstr->len; ++i) {
		if ((hint_index < blocks->hints_len) && (i == blocks->hints[hint_index].pos)) {
			if (blocks->hints[hint_index].value & FORMAT_BOLD_END) {
				wattroff(pager_window, A_BOLD);
			} else if (blocks->hints[hint_index].value & FORMAT_BOLD_BEGIN) {
				wattron(pager_window, A_BOLD);
			}
			if (blocks->hints[hint_index].value & FORMAT_UNDERLINED_END) {
				wattroff(pager_window, A_UNDERLINE);
			} else if (blocks->hints[hint_index].value & FORMAT_UNDERLINED_BEGIN) {
				wattron(pager_window, A_UNDERLINE);
			}
#ifdef A_ITALIC // Since A_ITALIC is an ncurses extension, some systems may lack it.
			if (blocks->hints[hint_index].value & FORMAT_ITALIC_END) {
				wattroff(pager_window, A_ITALIC);
			} else if (blocks->hints[hint_index].value & FORMAT_ITALIC_BEGIN) {
				wattron(pager_window, A_ITALIC);
			}
#endif
			hint_index += 1;
		}
		if (wstr->ptr[i] == '\n') {
			vertical_pos += 1;
			horizontal_pos = 0;
		} else {
			mvwaddnwstr(pager_window, vertical_pos, horizontal_pos++, wstr->ptr + i, 1);
		}
	}
	INFO("Wrote %zu lines to the content window.", vertical_pos);
}

static bool
update_pager_menu(void)
{
	struct wstring *text = render_data(blocks);
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

	pthread_mutex_lock(&interface_lock);

	if (pager_window != NULL) {
		delwin(pager_window);
	}

	INFO("Creating pad window with %zu width and %zu height.", list_menu_width, pad_height);
	pager_window = newpad(pad_height, list_menu_width);
	if (pager_window == NULL) {
		FAIL("Failed to create pad window for item contents (newpad returned NULL)!");
		free_wstring(text);
		pthread_mutex_unlock(&interface_lock);
		return false;
	}

	write_splitted_wstring_to_window(text);

	view_min = 0;
	view_lim = pad_height > list_menu_height ? pad_height - list_menu_height : 0;
	prefresh(pager_window, view_min, 0, 0, 0, list_menu_height - 1, list_menu_width - 1);

	pthread_mutex_unlock(&interface_lock);

	free_wstring(text);

	return true;
}

static inline void
scroll_view(size_t pminrow)
{
	if (pminrow != view_min) {
		view_min = pminrow;
		prefresh(pager_window, pminrow, 0, 0, 0, list_menu_height - 1, list_menu_width - 1);
	}
}

bool
start_pager_menu(struct render_blocks_list *new_blocks)
{
	blocks = new_blocks;
	pause_list_menu();
	if (update_pager_menu() == false) {
		// Error message written by update_pager_menu.
		resume_list_menu();
		return false;
	}
	return true;
}

bool
handle_pager_menu_navigation(input_cmd_id cmd)
{
	if ((cmd == INPUT_SELECT_NEXT) || (cmd == INPUT_ENTER)) { // Scroll one line down.
		scroll_view(view_min < view_lim ? view_min + 1 : view_lim);
	} else if (cmd == INPUT_SELECT_PREV) { // Scroll one line up.
		scroll_view(view_min == 0 ? 0 : view_min - 1);
	} else if (cmd == INPUT_SELECT_NEXT_PAGE) { // Scroll one page down.
		scroll_view(view_min + list_menu_height < view_lim ? view_min + list_menu_height : view_lim);
	} else if (cmd == INPUT_SELECT_PREV_PAGE) { // Scroll one page up.
		scroll_view(view_min > list_menu_height ? view_min - list_menu_height : 0);
	} else if (cmd == INPUT_SELECT_FIRST) { // Scroll to the beginning.
		scroll_view(0);
	} else if (cmd == INPUT_SELECT_LAST) { // Scroll to the end.
		scroll_view(view_lim);
	} else if (cmd == INPUT_RESIZE) {
		update_pager_menu();
	} else if ((cmd == INPUT_QUIT_SOFT) || (cmd == INPUT_QUIT_HARD)) {
		delwin(pager_window);
		pager_window = NULL;
		resume_list_menu();
		return false;
	} else {
		return false;
	}
	return true;
}
