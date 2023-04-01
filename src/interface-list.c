#include <stdlib.h>
#include <curses.h>
#include "newsraft.h"

// Note to the future.
// TODO: explain why we need all these *_unprotected functions

struct list_menu_settings {
	size_t entries_count; // Total number of entries.
	size_t view_sel; // Index of the selected entry.
	size_t view_min; // Index of the first visible entry.
	size_t view_max; // Index of the last visible entry.
	const struct wstring *entry_format;
	void (*read_action)(size_t index);
	void (*unread_action)(size_t index);
	const struct format_arg *(*get_args)(size_t index);
	int (*paint_action)(size_t index);
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
static volatile bool list_menu_is_paused = false;

int8_t
get_current_menu_type(void)
{
	return list_menu_is_paused == true ? MENUS_COUNT : menu - menus;
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

void
initialize_settings_of_list_menus(void)
{
	menus[SECTIONS_MENU].read_action   = &mark_section_read;
	menus[SECTIONS_MENU].unread_action = &mark_section_unread;
	menus[SECTIONS_MENU].get_args      = &get_section_entry_args;
	menus[SECTIONS_MENU].paint_action  = &paint_section_entry;
	menus[SECTIONS_MENU].hover_action  = NULL;
	menus[SECTIONS_MENU].unread_state  = &unread_section_condition;
	menus[FEEDS_MENU].read_action   = &mark_feed_read;
	menus[FEEDS_MENU].unread_action = &mark_feed_unread;
	menus[FEEDS_MENU].get_args      = &get_feed_entry_args;
	menus[FEEDS_MENU].paint_action  = &paint_feed_entry;
	menus[FEEDS_MENU].hover_action  = NULL;
	menus[FEEDS_MENU].unread_state  = &unread_feed_condition;
	menus[ITEMS_MENU].read_action   = &mark_item_read;
	menus[ITEMS_MENU].unread_action = &mark_item_unread;
	menus[ITEMS_MENU].get_args      = &get_item_entry_args;
	menus[ITEMS_MENU].paint_action  = &paint_item_entry;
	if (get_cfg_bool(CFG_MARK_ITEM_READ_ON_HOVER) == true) {
		menus[ITEMS_MENU].hover_action = &mark_item_read;
	} else {
		menus[ITEMS_MENU].hover_action = NULL;
	}
	menus[ITEMS_MENU].unread_state = &unread_item_condition;
}

static inline void
expose_entry_of_the_list_menu_unprotected(size_t index)
{
	WINDOW *w = windows[index - menu->view_min];
	werase(w);
	mvwaddnwstr(w, 0, 0, do_format(menu->entry_format, menu->get_args(index))->ptr, list_menu_width);
	if (index == menu->view_sel) {
		wbkgd(w, get_color_pair(menu->paint_action(index)) | A_REVERSE);
	} else {
		wbkgd(w, get_color_pair(menu->paint_action(index)));
	}
	wrefresh(w);
}

void
expose_entry_of_the_list_menu(size_t index)
{
	pthread_mutex_lock(&interface_lock);
	if ((list_menu_is_paused == false) && (index >= menu->view_min) && (index <= menu->view_max)) {
		expose_entry_of_the_list_menu_unprotected(index);
	}
	pthread_mutex_unlock(&interface_lock);
}

static inline void
expose_all_visible_entries_of_the_list_menu_unprotected(void)
{
	if (list_menu_is_paused == false) {
		for (size_t i = menu->view_min; i <= menu->view_max && i < menu->entries_count; ++i) {
			expose_entry_of_the_list_menu_unprotected(i);
		}
	}
}

void
expose_all_visible_entries_of_the_list_menu(void)
{
	pthread_mutex_lock(&interface_lock);
	expose_all_visible_entries_of_the_list_menu_unprotected();
	pthread_mutex_unlock(&interface_lock);
}

static inline void
erase_all_visible_entries_not_in_the_list_menu_unprotected(void)
{
	for (size_t i = menu->entries_count; i <= menu->view_max; ++i) {
		werase(windows[i - menu->view_min]);
		wbkgd(windows[i - menu->view_min], COLOR_PAIR(0));
		wnoutrefresh(windows[i - menu->view_min]);
	}
	doupdate();
}

void
redraw_list_menu_unprotected(void)
{
	if (list_menu_is_paused == false) {
		menu->view_max = menu->view_min + (list_menu_height - 1);
		if (menu->view_sel > menu->view_max) {
			menu->view_max = menu->view_sel;
			menu->view_min = menu->view_max - (list_menu_height - 1);
		}
		expose_all_visible_entries_of_the_list_menu_unprotected();
		erase_all_visible_entries_not_in_the_list_menu_unprotected();
	}
}

const size_t *
enter_list_menu(int8_t menu_index, size_t new_entries_count, config_entry_id format_id)
{
	pthread_mutex_lock(&interface_lock);
	menu = menus + menu_index;
	if (new_entries_count != 0) {
		menu->entries_count = new_entries_count;
		menu->view_sel = 0;
		menu->view_min = 0;
		menu->entry_format = get_cfg_wstring(format_id);
	}
	if (menu_index == SECTIONS_MENU) {
		refresh_unread_items_count_of_all_sections();
	}
	redraw_list_menu_unprotected();
	pthread_mutex_unlock(&interface_lock);
	return &(menu->view_sel);
}

void
reset_list_menu_unprotected(size_t new_entries_count)
{
	if (new_entries_count < menu->entries_count) {
		menu->view_sel = 0;
		menu->view_min = 0;
	}
	menu->entries_count = new_entries_count;
	redraw_list_menu_unprotected();
}

void
pause_list_menu(void)
{
	pthread_mutex_lock(&interface_lock);
	list_menu_is_paused = true;
	clear();
	refresh();
	status_clean_unprotected();
	status_recreate_unprotected();
	pthread_mutex_unlock(&interface_lock);
}

void
resume_list_menu(void)
{
	pthread_mutex_lock(&interface_lock);
	list_menu_is_paused = false;
	clear();
	refresh();
	redraw_list_menu_unprotected();
	status_clean_unprotected();
	status_recreate_unprotected();
	pthread_mutex_unlock(&interface_lock);
}

static void
list_menu_change_view(size_t new_sel)
{
	// Gotta lock right away because we use menu->entries_count below.
	pthread_mutex_lock(&interface_lock);

	if (new_sel >= menu->entries_count) {
		if (menu->entries_count == 0) {
			pthread_mutex_unlock(&interface_lock);
			return;
		}
		new_sel = menu->entries_count - 1;
	}

	if ((new_sel + scrolloff) > menu->view_max) {
		if (menu->entries_count > list_menu_height) {
			menu->view_max = MIN(new_sel + scrolloff, menu->entries_count - 1);
		} else {
			menu->view_max = list_menu_height - 1;
		}
		menu->view_min = menu->view_max - (list_menu_height - 1);
		menu->view_sel = new_sel;
		expose_all_visible_entries_of_the_list_menu_unprotected();
	} else if ((new_sel >= scrolloff) && ((new_sel - scrolloff) < menu->view_min)) {
		menu->view_min = new_sel - scrolloff;
		menu->view_max = menu->view_min + (list_menu_height - 1);
		menu->view_sel = new_sel;
		expose_all_visible_entries_of_the_list_menu_unprotected();
	} else if (new_sel < menu->view_min) {
		menu->view_min = new_sel < scrolloff ? 0 : new_sel - scrolloff;
		menu->view_max = menu->view_min + (list_menu_height - 1);
		menu->view_sel = new_sel;
		expose_all_visible_entries_of_the_list_menu_unprotected();
	} else if (new_sel != menu->view_sel) {
		if (list_menu_is_paused == true) {
			menu->view_sel = new_sel;
		} else {
			WINDOW *w = windows[menu->view_sel - menu->view_min];
			wbkgd(w, get_color_pair(menu->paint_action(menu->view_sel)));
			wrefresh(w);
			menu->view_sel = new_sel;
			w = windows[menu->view_sel - menu->view_min];
			wbkgd(w, get_color_pair(menu->paint_action(menu->view_sel)) | A_REVERSE);
			wrefresh(w);
		}
	}

	pthread_mutex_unlock(&interface_lock);

	// We have to perform the hover action only after unlocking the interface
	// lock because the hover actions are already thread-safe functions.
	if (menu->hover_action != NULL) {
		menu->hover_action(menu->view_sel);
	}
}

bool
handle_list_menu_navigation(input_cmd_id cmd)
{
	if ((cmd == INPUT_SELECT_NEXT) || (cmd == INPUT_JUMP_TO_NEXT)) {
		list_menu_change_view(menu->view_sel + 1);
	} else if ((cmd == INPUT_SELECT_PREV) || (cmd == INPUT_JUMP_TO_PREV)) {
		list_menu_change_view(menu->view_sel > 1 ? (menu->view_sel - 1) : 0);
	} else if (cmd == INPUT_JUMP_TO_NEXT_UNREAD) {
		for (size_t i = 1, j = menu->view_sel + 1; i < menu->entries_count; ++i, ++j) {
			j %= menu->entries_count;
			if (menu->unread_state(j) == true) {
				list_menu_change_view(j);
				break;
			}
		}
	} else if (cmd == INPUT_JUMP_TO_PREV_UNREAD) {
		for (size_t i = 1, j = menu->view_sel; i < menu->entries_count; ++i, --j) {
			if (j == 0) {
				j = menu->entries_count;
			}
			if (menu->unread_state(j - 1) == true) {
				list_menu_change_view(j - 1);
				break;
			}
		}
	} else if ((cmd == INPUT_JUMP_TO_NEXT_IMPORTANT) && (menu - menus == ITEMS_MENU)) {
		for (size_t i = 1, j = menu->view_sel + 1; i < menu->entries_count; ++i, ++j) {
			j %= menu->entries_count;
			if (important_item_condition(j) == true) {
				list_menu_change_view(j);
				break;
			}
		}
	} else if ((cmd == INPUT_JUMP_TO_PREV_IMPORTANT) && (menu - menus == ITEMS_MENU)) {
		for (size_t i = 1, j = menu->view_sel; i < menu->entries_count; ++i, --j) {
			if (j == 0) {
				j = menu->entries_count;
			}
			if (important_item_condition(j - 1) == true) {
				list_menu_change_view(j - 1);
				break;
			}
		}
	} else if (cmd == INPUT_SELECT_NEXT_PAGE) {
		list_menu_change_view(menu->view_sel + list_menu_height);
	} else if (cmd == INPUT_SELECT_PREV_PAGE) {
		list_menu_change_view(menu->view_sel > list_menu_height ? (menu->view_sel - list_menu_height) : 0);
	} else if (cmd == INPUT_SELECT_FIRST) {
		list_menu_change_view(0);
	} else if (cmd == INPUT_SELECT_LAST) {
		list_menu_change_view(menu->entries_count > 1 ? (menu->entries_count - 1) : 0);
	} else if (cmd == INPUT_MARK_READ) {
		menu->read_action(menu->view_sel);
	} else if (cmd == INPUT_MARK_READ_AND_JUMP_TO_NEXT) {
		menu->read_action(menu->view_sel);
		handle_list_menu_navigation(INPUT_SELECT_NEXT);
	} else if (cmd == INPUT_MARK_READ_AND_JUMP_TO_NEXT_UNREAD) {
		menu->read_action(menu->view_sel);
		handle_list_menu_navigation(INPUT_JUMP_TO_NEXT_UNREAD);
	} else if (cmd == INPUT_MARK_UNREAD) {
		menu->unread_action(menu->view_sel);
	} else if (cmd == INPUT_MARK_UNREAD_AND_JUMP_TO_NEXT) {
		menu->unread_action(menu->view_sel);
		handle_list_menu_navigation(INPUT_SELECT_NEXT);
	} else {
		return false;
	}
	return true;
}
