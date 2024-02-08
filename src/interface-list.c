#include <stdlib.h>
#include "newsraft.h"

// Note to the future.
// TODO: explain why we need all these *_unprotected functions

struct list_menu_settings {
	size_t view_sel; // Index of the selected entry.
	size_t view_min; // Index of the first visible entry.
	size_t view_max; // Index of the last visible entry.
	const struct wstring *entry_format;
	bool (*enumerator)(size_t index); // Checks if index is valid.
	const struct format_arg *(*get_args)(size_t index);
	int (*paint_action)(size_t index);
	void (*write_action)(size_t index, WINDOW *w);
	bool (*unread_state)(size_t index);
};

size_t list_menu_height;
size_t list_menu_width;

static WINDOW **windows = NULL;
static size_t windows_count = 0;
static size_t scrolloff;
static size_t horizontal_shift = 0;

static void list_menu_writer(size_t index, WINDOW *w);

static struct list_menu_settings menus[MENUS_COUNT] = {
	{0, 0, 0, NULL, &is_section_valid,   &get_section_args, &paint_section, &list_menu_writer,  &is_section_unread},
	{0, 0, 0, NULL, &is_feed_valid,      &get_feed_args,    &paint_feed,    &list_menu_writer,  &is_feed_unread},
	{0, 0, 0, NULL, &is_item_valid,      &get_item_args,    &paint_item,    &list_menu_writer,  &is_item_unread},
	{0, 0, 0, NULL, &is_pager_pos_valid, NULL,              NULL,           &pager_menu_writer, NULL},
};
static struct list_menu_settings *menu = menus; // Selected menu.

static struct wstring *list_fmtout = NULL;

int8_t
get_current_menu_type(void)
{
	return menu - menus;
}

bool
adjust_list_menu(void)
{
	INFO("Adjusting list menu.");
	// Delete old windows to create new windows with the new size because
	// useful in this case resizing is a non-standard curses extension.
	for (size_t i = 0; i < windows_count; ++i) {
		delwin(windows[i]);
	}
	WINDOW **temp = realloc(windows, sizeof(WINDOW *) * list_menu_height);
	if (temp == NULL) {
		goto error;
	}
	windows = temp;
	for (size_t i = 0; i < list_menu_height; ++i) {
		windows[i] = newwin(1, list_menu_width, i, 0);
		if (windows[i] == NULL) {
			windows_count = i;
			goto error;
		}
	}
	windows_count = list_menu_height;
	scrolloff = get_cfg_uint(CFG_SCROLLOFF);
	if (scrolloff > (list_menu_height / 2)) {
		scrolloff = list_menu_height / 2;
	}
	if (wstr_set(&list_fmtout, NULL, 0, 200) == false) {
		goto error;
	}
	return true;
error:
	FAIL("Not enough memory for adjusting list menu!");
	return false;
}

void
free_list_menu(void)
{
	for (size_t i = 0; i < windows_count; ++i) {
		delwin(windows[i]);
	}
	free(windows);
	free_wstring(list_fmtout);
}

static void
list_menu_writer(size_t index, WINDOW *w)
{
	if (menu->enumerator(index) == true) {
		do_format(list_fmtout, menu->entry_format->ptr, menu->get_args(index));
		if (list_fmtout->len > horizontal_shift) {
			waddwstr(w, list_fmtout->ptr + horizontal_shift);
		}
		wbkgd(w, get_color_pair(menu->paint_action(index)) | (index == menu->view_sel ? A_REVERSE : 0));
	}
}

static inline void
expose_entry_of_the_list_menu_unprotected(size_t index)
{
	WINDOW *w = windows[index - menu->view_min];
	werase(w);
	wmove(w, 0, 0);
	wbkgd(w, A_NORMAL);
	wattrset(w, A_NORMAL);
	menu->write_action(index, w);
	wnoutrefresh(w);
}

void
expose_entry_of_the_list_menu(size_t index)
{
	pthread_mutex_lock(&interface_lock);
	if (index >= menu->view_min && index <= menu->view_max) {
		expose_entry_of_the_list_menu_unprotected(index);
		doupdate();
	}
	pthread_mutex_unlock(&interface_lock);
}

static inline void
expose_all_visible_entries_of_the_list_menu_unprotected(void)
{
	for (size_t i = menu->view_min; i <= menu->view_max; ++i) {
		expose_entry_of_the_list_menu_unprotected(i);
	}
	doupdate();
}

void
expose_all_visible_entries_of_the_list_menu(void)
{
	pthread_mutex_lock(&interface_lock);
	expose_all_visible_entries_of_the_list_menu_unprotected();
	pthread_mutex_unlock(&interface_lock);
}

void
redraw_list_menu_unprotected(void)
{
	menu->view_max = menu->view_min + (list_menu_height - 1);
	if (menu->view_sel > menu->view_max) {
		menu->view_max = menu->view_sel;
		menu->view_min = menu->view_max - (list_menu_height - 1);
	}
	while (menu->view_max >= list_menu_height && menu->enumerator(menu->view_max) == false) {
		menu->view_max -= 1;
	}
	menu->view_min = menu->view_max - (list_menu_height - 1);
	expose_all_visible_entries_of_the_list_menu_unprotected();
}

const size_t *
enter_list_menu(int8_t menu_index, config_entry_id format_id, bool do_reset)
{
	pthread_mutex_lock(&interface_lock);
	menu = menus + menu_index;
	// Always reset entry_format because sometimes we enter list menu for
	// the first time without do_reset set to true!
	menu->entry_format = get_cfg_wstring(format_id);
	horizontal_shift = 0;
	if (do_reset != false) {
		menu->view_sel = 0;
		menu->view_min = 0;
		status_clean_unprotected();
	}
	redraw_list_menu_unprotected();
	pthread_mutex_unlock(&interface_lock);
	return &(menu->view_sel);
}

void
reset_list_menu_unprotected(void)
{
	if (menu->enumerator(menu->view_sel) == false) {
		menu->view_sel = 0;
		menu->view_min = 0;
	}
	redraw_list_menu_unprotected();
}

static size_t
obtain_list_entries_count_unprotected(struct list_menu_settings *m)
{
	size_t i = 0;
	while (m->enumerator(i) == true) {
		i += 1;
	}
	return i;
}

static void
change_list_view_unprotected(struct list_menu_settings *m, size_t new_sel)
{
	while (m->enumerator(new_sel) == false && new_sel > 0) {
		new_sel -= 1;
	}
	if (m->enumerator(new_sel) == false) {
		return;
	}
	if (new_sel + scrolloff > m->view_max) {
		m->view_max = new_sel + scrolloff;
		while (m->view_max >= list_menu_height && m->enumerator(m->view_max) == false) {
			m->view_max -= 1;
		}
		m->view_min = m->view_max - (list_menu_height - 1);
		m->view_sel = new_sel;
		if (m == menu) {
			expose_all_visible_entries_of_the_list_menu_unprotected();
		}
	} else if (((new_sel >= scrolloff) && (new_sel - scrolloff < m->view_min))
		|| ((new_sel < scrolloff) && (m->view_min > 0)))
	{
		if (new_sel >= scrolloff) {
			m->view_min = new_sel - scrolloff;
			if ((scrolloff == list_menu_height / 2) && (list_menu_height % 2 == 0)) {
				// Makes scrolling with huge scrolloff work consistently in both direcetions.
				m->view_min += 1;
			}
		} else {
			m->view_min = 0;
		}
		m->view_max = m->view_min + (list_menu_height - 1);
		m->view_sel = new_sel;
		if (m == menu) {
			expose_all_visible_entries_of_the_list_menu_unprotected();
		}
	} else if (new_sel != m->view_sel) {
		if (m == menu) {
			WINDOW *w = windows[m->view_sel - m->view_min];
			wbkgd(w, get_color_pair(m->paint_action(m->view_sel)));
			wnoutrefresh(w);
			m->view_sel = new_sel;
			w = windows[m->view_sel - m->view_min];
			wbkgd(w, get_color_pair(m->paint_action(m->view_sel)) | A_REVERSE);
			wnoutrefresh(w);
			doupdate();
		} else {
			m->view_sel = new_sel;
		}
	}
}

bool
handle_list_menu_control(uint8_t menu_id, input_cmd_id cmd, const struct wstring *arg)
{
	pthread_mutex_lock(&interface_lock);
	struct list_menu_settings *m = &menus[menu_id];
	if (cmd == INPUT_SELECT_NEXT || cmd == INPUT_JUMP_TO_NEXT) {
		change_list_view_unprotected(m, m->view_sel + 1);
	} else if (cmd == INPUT_SELECT_PREV || cmd == INPUT_JUMP_TO_PREV) {
		change_list_view_unprotected(m, m->view_sel > 1 ? m->view_sel - 1 : 0);
	} else if (cmd == INPUT_JUMP_TO_NEXT_UNREAD) {
		for (size_t i = m->view_sel + 1, j = 0; j < 2; i = 0, ++j) {
			while (m->enumerator(i) == true) {
				if (m->unread_state(i) == true) {
					change_list_view_unprotected(m, i);
					j = 2;
					break;
				}
				i += 1;
			}
		}
	} else if (cmd == INPUT_JUMP_TO_PREV_UNREAD) {
		for (size_t i = m->view_sel, j = 0; j < 2; ++j) {
			while ((i > 0) && (m->enumerator(i - 1) == true)) {
				if (m->unread_state(i - 1) == true) {
					change_list_view_unprotected(m, i - 1);
					j = 2;
					break;
				}
				i -= 1;
			}
			if (j < 2) {
				i = obtain_list_entries_count_unprotected(m);
			}
		}
	} else if (cmd == INPUT_JUMP_TO_NEXT_IMPORTANT && menu_id == ITEMS_MENU) {
		for (size_t i = m->view_sel + 1, j = 0; j < 2; i = 0, ++j) {
			while (m->enumerator(i) == true) {
				if (important_item_condition(i) == true) {
					change_list_view_unprotected(m, i);
					j = 2;
					break;
				}
				i += 1;
			}
		}
	} else if (cmd == INPUT_JUMP_TO_PREV_IMPORTANT && menu_id == ITEMS_MENU) {
		for (size_t i = m->view_sel, j = 0; j < 2; ++j) {
			while (i > 0 && m->enumerator(i - 1) == true) {
				if (important_item_condition(i - 1) == true) {
					change_list_view_unprotected(m, i - 1);
					j = 2;
					break;
				}
				i -= 1;
			}
			if (j < 2) {
				i = obtain_list_entries_count_unprotected(m);
			}
		}
	} else if (cmd == INPUT_SELECT_NEXT_PAGE) {
		change_list_view_unprotected(m, m->view_sel + list_menu_height);
	} else if (cmd == INPUT_SELECT_PREV_PAGE) {
		change_list_view_unprotected(m, m->view_sel > list_menu_height ? m->view_sel - list_menu_height : 0);
	} else if (cmd == INPUT_SELECT_FIRST) {
		change_list_view_unprotected(m, 0);
	} else if (cmd == INPUT_SELECT_LAST) {
		change_list_view_unprotected(m, obtain_list_entries_count_unprotected(m));
	} else if (cmd == INPUT_SHIFT_WEST) {
		size_t shift_delta = 1 + list_menu_width / 50;
		if (horizontal_shift >= shift_delta) {
			horizontal_shift -= shift_delta;
			expose_all_visible_entries_of_the_list_menu_unprotected();
		} else if (horizontal_shift > 0) {
			horizontal_shift = 0;
			expose_all_visible_entries_of_the_list_menu_unprotected();
		}
	} else if (cmd == INPUT_SHIFT_EAST) {
		horizontal_shift += 1 + list_menu_width / 50;
		expose_all_visible_entries_of_the_list_menu_unprotected();
	} else if (cmd == INPUT_SHIFT_RESET) {
		if (horizontal_shift != 0) {
			horizontal_shift = 0;
			expose_all_visible_entries_of_the_list_menu_unprotected();
		}
	} else if (cmd == INPUT_SYSTEM_COMMAND) {
		pthread_mutex_unlock(&interface_lock);
		run_formatted_command(arg, m->get_args(m->view_sel));
		return true;
	} else {
		pthread_mutex_unlock(&interface_lock);
		return false;
	}
	pthread_mutex_unlock(&interface_lock);
	return true;
}

static void
change_pager_view_unprotected(size_t new_sel)
{
	while (menu->enumerator(new_sel) == false && new_sel > 0) {
		new_sel -= 1;
	}
	if (menu->enumerator(new_sel) == false) {
		return;
	}
	size_t entries_count = obtain_list_entries_count_unprotected(menu);
	if (entries_count - new_sel < list_menu_height) {
		new_sel = entries_count > list_menu_height ? entries_count - list_menu_height : 0;
	}
	if (new_sel != menu->view_min) {
		menu->view_max = new_sel + (list_menu_height - 1);
		while (menu->view_max >= list_menu_height && menu->enumerator(menu->view_max) == false) {
			menu->view_max -= 1;
		}
		menu->view_min = menu->view_max - (list_menu_height - 1);
		expose_all_visible_entries_of_the_list_menu_unprotected();
	}
}

bool
handle_pager_menu_control(input_cmd_id cmd)
{
	pthread_mutex_lock(&interface_lock);
	if (cmd == INPUT_SELECT_NEXT || cmd == INPUT_ENTER) {
		change_pager_view_unprotected(menu->view_min + 1);
	} else if (cmd == INPUT_SELECT_PREV) {
		change_pager_view_unprotected(menu->view_min > 0 ? menu->view_min - 1 : 0);
	} else if (cmd == INPUT_SELECT_NEXT_PAGE) {
		change_pager_view_unprotected(menu->view_min + list_menu_height);
	} else if (cmd == INPUT_SELECT_PREV_PAGE) {
		change_pager_view_unprotected(menu->view_min > list_menu_height ? menu->view_min - list_menu_height : 0);
	} else if (cmd == INPUT_SELECT_FIRST) {
		change_pager_view_unprotected(0);
	} else if (cmd == INPUT_SELECT_LAST) {
		change_pager_view_unprotected(obtain_list_entries_count_unprotected(menu));
	} else {
		pthread_mutex_unlock(&interface_lock);
		return false;
	}
	pthread_mutex_unlock(&interface_lock);
	return true;
}
