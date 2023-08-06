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
	void (*read_action)(size_t index);
	void (*unread_action)(size_t index);
	const struct format_arg *(*get_args)(size_t index);
	int (*paint_action)(size_t index);
	void (*write_action)(size_t index, WINDOW *w);
	void (*hover_action)(size_t index);
	bool (*unread_state)(size_t index);
};

size_t list_menu_height;
size_t list_menu_width;

static WINDOW **windows = NULL;
static size_t windows_count = 0;
static size_t scrolloff;

static struct list_menu_settings menus[MENUS_COUNT];
static struct list_menu_settings *menu = menus; // Selected menu.

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
}

static void
regular_list_menu_writer(size_t index, WINDOW *w)
{
	if (menu->enumerator(index) == true) {
		waddnwstr(w, do_format(menu->entry_format->ptr, menu->get_args(index))->ptr, list_menu_width);
		wbkgd(w, get_color_pair(menu->paint_action(index)) | (index == menu->view_sel ? A_REVERSE : 0));
	}
}

void
initialize_settings_of_list_menus(void)
{
	menus[SECTIONS_MENU].enumerator    = &sections_list_moderator;
	menus[SECTIONS_MENU].read_action   = &mark_section_read;
	menus[SECTIONS_MENU].unread_action = &mark_section_unread;
	menus[SECTIONS_MENU].get_args      = &get_section_entry_args;
	menus[SECTIONS_MENU].paint_action  = &paint_section_entry;
	menus[SECTIONS_MENU].write_action  = &regular_list_menu_writer;
	menus[SECTIONS_MENU].hover_action  = NULL;
	menus[SECTIONS_MENU].unread_state  = &unread_section_condition;
	menus[FEEDS_MENU].enumerator    = &feeds_list_moderator;
	menus[FEEDS_MENU].read_action   = &mark_feed_read;
	menus[FEEDS_MENU].unread_action = &mark_feed_unread;
	menus[FEEDS_MENU].get_args      = &get_feed_entry_args;
	menus[FEEDS_MENU].paint_action  = &paint_feed_entry;
	menus[FEEDS_MENU].write_action  = &regular_list_menu_writer;
	menus[FEEDS_MENU].hover_action  = NULL;
	menus[FEEDS_MENU].unread_state  = &unread_feed_condition;
	menus[ITEMS_MENU].enumerator    = &items_list_moderator;
	menus[ITEMS_MENU].read_action   = &mark_item_read;
	menus[ITEMS_MENU].unread_action = &mark_item_unread;
	menus[ITEMS_MENU].get_args      = &get_item_entry_args;
	menus[ITEMS_MENU].paint_action  = &paint_item_entry;
	menus[ITEMS_MENU].write_action  = &regular_list_menu_writer;
	if (get_cfg_bool(CFG_MARK_ITEM_READ_ON_HOVER) == true) {
		menus[ITEMS_MENU].hover_action = &mark_item_read;
	} else {
		menus[ITEMS_MENU].hover_action = NULL;
	}
	menus[ITEMS_MENU].unread_state  = &unread_item_condition;
	menus[PAGER_MENU].enumerator    = &pager_menu_moderator;
	menus[PAGER_MENU].write_action  = &pager_list_menu_writer;
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
	wrefresh(w);
}

void
expose_entry_of_the_list_menu(size_t index)
{
	pthread_mutex_lock(&interface_lock);
	if (index >= menu->view_min && index <= menu->view_max) {
		expose_entry_of_the_list_menu_unprotected(index);
	}
	pthread_mutex_unlock(&interface_lock);
}

static inline void
expose_all_visible_entries_of_the_list_menu_unprotected(void)
{
	for (size_t i = menu->view_min; i <= menu->view_max; ++i) {
		expose_entry_of_the_list_menu_unprotected(i);
	}
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
	struct list_menu_settings *prev_menu = menu;
	menu = menus + menu_index;
	if (do_reset == true) {
		menu->view_sel = 0;
		menu->view_min = 0;
		menu->entry_format = get_cfg_wstring(format_id);
	}
	if (menu_index == SECTIONS_MENU) {
		refresh_unread_items_count_of_all_sections();
	}
	if (do_reset == true || menu != prev_menu) {
		status_clean_unprotected();
		redraw_list_menu_unprotected();
	}
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

static size_t
obtain_list_entries_count(struct list_menu_settings *m)
{
	pthread_mutex_lock(&interface_lock);
	size_t count = obtain_list_entries_count_unprotected(m);
	pthread_mutex_unlock(&interface_lock);
	return count;
}

static void
list_menu_change_view(struct list_menu_settings *m, size_t new_sel)
{
	pthread_mutex_lock(&interface_lock);

	while (m->enumerator(new_sel) == false && new_sel > 0) {
		new_sel -= 1;
	}
	if (m->enumerator(new_sel) == false) {
		pthread_mutex_unlock(&interface_lock);
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
			wrefresh(w);
			m->view_sel = new_sel;
			w = windows[m->view_sel - m->view_min];
			wbkgd(w, get_color_pair(m->paint_action(m->view_sel)) | A_REVERSE);
			wrefresh(w);
		} else {
			m->view_sel = new_sel;
		}
	}

	pthread_mutex_unlock(&interface_lock);

	// We have to perform the hover action only after unlocking the interface
	// lock because the hover actions are already thread-safe functions.
	if (m->hover_action != NULL) {
		m->hover_action(m->view_sel);
	}
}

bool
handle_list_menu_control(uint8_t menu_id, input_cmd_id cmd, const struct wstring *arg)
{
	struct list_menu_settings *m = &menus[menu_id];
	if (cmd == INPUT_SELECT_NEXT || cmd == INPUT_JUMP_TO_NEXT) {
		list_menu_change_view(m, m->view_sel + 1);
	} else if (cmd == INPUT_SELECT_PREV || cmd == INPUT_JUMP_TO_PREV) {
		list_menu_change_view(m, m->view_sel > 1 ? m->view_sel - 1 : 0);
	} else if (cmd == INPUT_JUMP_TO_NEXT_UNREAD) {
		for (size_t i = m->view_sel + 1, j = 0; j < 2; i = 0, ++j) {
			while (m->enumerator(i) == true) {
				if (m->unread_state(i) == true) {
					list_menu_change_view(m, i);
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
					list_menu_change_view(m, i - 1);
					j = 2;
					break;
				}
				i -= 1;
			}
			if (j < 2) {
				i = obtain_list_entries_count(m);
			}
		}
	} else if (cmd == INPUT_JUMP_TO_NEXT_IMPORTANT && menu_id == ITEMS_MENU) {
		for (size_t i = m->view_sel + 1, j = 0; j < 2; i = 0, ++j) {
			while (m->enumerator(i) == true) {
				if (important_item_condition(i) == true) {
					list_menu_change_view(m, i);
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
					list_menu_change_view(m, i - 1);
					j = 2;
					break;
				}
				i -= 1;
			}
			if (j < 2) {
				i = obtain_list_entries_count(m);
			}
		}
	} else if (cmd == INPUT_SELECT_NEXT_PAGE) {
		list_menu_change_view(m, m->view_sel + list_menu_height);
	} else if (cmd == INPUT_SELECT_PREV_PAGE) {
		list_menu_change_view(m, m->view_sel > list_menu_height ? m->view_sel - list_menu_height : 0);
	} else if (cmd == INPUT_SELECT_FIRST) {
		list_menu_change_view(m, 0);
	} else if (cmd == INPUT_SELECT_LAST) {
		list_menu_change_view(m, obtain_list_entries_count(m));
	} else if (cmd == INPUT_MARK_READ) {
		m->read_action(m->view_sel);
	} else if (cmd == INPUT_MARK_UNREAD) {
		m->unread_action(m->view_sel);
	} else if (cmd == INPUT_SYSTEM_COMMAND) {
		run_command_with_specifiers(arg, m->get_args(m->view_sel));
	} else {
		return false;
	}
	return true;
}

static void
pager_menu_change_view(size_t new_sel)
{
	pthread_mutex_lock(&interface_lock);

	while (menu->enumerator(new_sel) == false && new_sel > 0) {
		new_sel -= 1;
	}
	if (menu->enumerator(new_sel) == false) {
		pthread_mutex_unlock(&interface_lock);
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

	pthread_mutex_unlock(&interface_lock);
}

bool
handle_pager_menu_control(input_cmd_id cmd)
{
	if (cmd == INPUT_SELECT_NEXT || cmd == INPUT_ENTER) {
		pager_menu_change_view(menu->view_min + 1);
	} else if (cmd == INPUT_SELECT_PREV) {
		pager_menu_change_view(menu->view_min > 0 ? menu->view_min - 1 : 0);
	} else if (cmd == INPUT_SELECT_NEXT_PAGE) {
		pager_menu_change_view(menu->view_min + list_menu_height);
	} else if (cmd == INPUT_SELECT_PREV_PAGE) {
		pager_menu_change_view(menu->view_min > list_menu_height ? menu->view_min - list_menu_height : 0);
	} else if (cmd == INPUT_SELECT_FIRST) {
		pager_menu_change_view(0);
	} else if (cmd == INPUT_SELECT_LAST) {
		pager_menu_change_view(obtain_list_entries_count(menu));
	} else if (cmd == INPUT_RESIZE) {
		reset_pager_menu();
	} else {
		return false;
	}
	return true;
}
