#include <stdlib.h>
#include "feedeater.h"

static WINDOW **windows = NULL;
static size_t windows_count = 0;
size_t list_menu_height;
size_t list_menu_width;

void
free_list_menu(void)
{
	for (size_t i = 0; i < windows_count; ++i) {
		delwin(windows[i]);
	}
	free(windows);
}

bool
adjust_list_menu(void)
{
	if (list_menu_height > windows_count) {
		WINDOW **temp = realloc(windows, sizeof(WINDOW *) * list_menu_height);
		if (temp == NULL) {
			FAIL("Not enough memory for reallocating list menu windows!");
			return false;
		}
		windows = temp;
		for (size_t i = windows_count; i < list_menu_height; ++i) {
			windows[i] = newwin(1, list_menu_width, i, 0);
			if (windows[i] == NULL) {
				FAIL("Not enough memory for window to adjust list menu!");
				windows_count = i;
				free_list_menu();

				// Set these to 0 because free_list_menu() will
				// be called again in the end of main(). That way we will
				// avoid double free. Yeah, yeah, looks "very" ineffective,
				// but what are the odds of shortage of memory on list menu
				// reallocation?
				windows_count = 0;
				windows = NULL;

				return false;
			}
		}
	}
	// if (list_menu_height < windows_count) {
	//     You might think that these windows that are not needed anymore should be deleted,
	//     but this function is called on screen resize, on which ncurses automatically
	//     deletes everything that entirely gone out of bounds. Hence do nothing in this case...
	// }
	windows_count = list_menu_height;
	return true;
}

WINDOW *
get_list_entry_by_index(size_t index)
{
	if (index < windows_count) {
		return windows[index];
	}
	return NULL;
}

void
expose_entry_of_the_menu_list(struct menu_list_settings *settings, size_t index)
{
	size_t target_window = (index - settings->view_min) % list_menu_height;
	werase(windows[target_window]);
	mvwaddnwstr(windows[target_window], 0, 0, settings->paint_action(index), list_menu_width);
	mvwchgat(windows[target_window], 0, 0, -1, (index == settings->view_sel) ? A_REVERSE : A_NORMAL, 0, NULL);
	wrefresh(windows[target_window]);
}

static inline void
expose_all_visible_entries_of_the_menu_list(struct menu_list_settings *settings)
{
	for (size_t i = settings->view_min; i < settings->entries_count && i <= settings->view_max; ++i) {
		expose_entry_of_the_menu_list(settings, i);
	}
}

void
redraw_menu_list(struct menu_list_settings *settings)
{
	clear();
	refresh();
	status_update();
	settings->view_max = settings->view_min + (list_menu_height - 1);
	if (settings->view_max < settings->view_sel) {
		settings->view_max = settings->view_sel;
		settings->view_min = settings->view_max - (list_menu_height - 1);
	}
	expose_all_visible_entries_of_the_menu_list(settings);
}

static void
list_menu_change_view(struct menu_list_settings *s, size_t i)
{
	size_t new_sel = i;

	if (new_sel >= s->entries_count) {
		if (s->entries_count == 0) {
			return;
		}
		new_sel = s->entries_count - 1;
	}

	if (new_sel == s->view_sel) {
		return;
	}

	if (new_sel > s->view_max) {
		s->view_min = new_sel - (list_menu_height - 1);
		s->view_max = new_sel;
		s->view_sel = new_sel;
		expose_all_visible_entries_of_the_menu_list(s);
	} else if (new_sel < s->view_min) {
		s->view_min = new_sel;
		s->view_max = new_sel + (list_menu_height - 1);
		s->view_sel = new_sel;
		expose_all_visible_entries_of_the_menu_list(s);
	} else {
		size_t target_window = (s->view_sel - s->view_min) % list_menu_height;
		mvwchgat(windows[target_window], 0, 0, -1, A_NORMAL, 0, NULL);
		wrefresh(windows[target_window]);
		s->view_sel = new_sel;
		target_window = (s->view_sel - s->view_min) % list_menu_height;
		mvwchgat(windows[target_window], 0, 0, -1, A_REVERSE, 0, NULL);
		wrefresh(windows[target_window]);
	}
}

void
list_menu_select_next(struct menu_list_settings *s)
{
	list_menu_change_view(s, s->view_sel + 1);
}

void
list_menu_select_prev(struct menu_list_settings *s)
{
	list_menu_change_view(s, (s->view_sel > 1) ? (s->view_sel - 1) : (0));
}

void
list_menu_select_next_page(struct menu_list_settings *s)
{
	list_menu_change_view(s, s->view_sel + list_menu_height);
}

void
list_menu_select_prev_page(struct menu_list_settings *s)
{
	list_menu_change_view(s, (s->view_sel > list_menu_height) ? (s->view_sel - list_menu_height) : (0));
}

void
list_menu_select_first(struct menu_list_settings *s)
{
	list_menu_change_view(s, 0);
}

void
list_menu_select_last(struct menu_list_settings *s)
{
	list_menu_change_view(s, (s->entries_count > 1) ? (s->entries_count - 1) : (0));
}
