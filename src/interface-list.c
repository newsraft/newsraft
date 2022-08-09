#include <stdlib.h>
#include "newsraft.h"

struct list_menu_settings {
	size_t entries_count; // Total number of entries.
	size_t view_sel; // Index of the selected entry.
	size_t view_min; // Index of the first visible entry.
	size_t view_max; // Index of the last visible entry.
	const wchar_t *(*write_action)(size_t index);
	int (*paint_action)(size_t index);
	void (*hover_action)(void);
	bool (*unread_state)(size_t index);
};

size_t list_menu_height;
size_t list_menu_width;

static WINDOW **windows = NULL;
static size_t windows_count = 0;

static struct list_menu_settings menus[MENUS_COUNT];
static struct list_menu_settings *menu = menus; // selected menu
static int8_t menus_immersion[10];
static int8_t menus_immersion_depth = 0;
static bool list_menu_is_paused = false;

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
	menus[SECTIONS_MENU].write_action = &write_section_entry;
	menus[SECTIONS_MENU].paint_action = &paint_section_entry;
	menus[SECTIONS_MENU].hover_action = NULL;
	menus[SECTIONS_MENU].unread_state = &unread_section_condition;
	menus[FEEDS_MENU].write_action = &write_feed_entry;
	menus[FEEDS_MENU].paint_action = &paint_feed_entry;
	menus[FEEDS_MENU].hover_action = NULL;
	menus[FEEDS_MENU].unread_state = &unread_feed_condition;
	menus[ITEMS_MENU].write_action = &write_item_entry;
	menus[ITEMS_MENU].paint_action = &paint_item_entry;
	if (get_cfg_bool(CFG_MARK_ITEM_READ_ON_HOVER) == true) {
		menus[ITEMS_MENU].hover_action = &mark_selected_item_read;
	} else {
		menus[ITEMS_MENU].hover_action = NULL;
	}
	menus[ITEMS_MENU].unread_state = &unread_item_condition;
}

void
expose_entry_of_the_list_menu(size_t index)
{
	if ((list_menu_is_paused == false) && (index >= menu->view_min) && (index <= menu->view_max)) {
		WINDOW *w = windows[index - menu->view_min];
		werase(w);
		mvwaddnwstr(w, 0, 0, menu->write_action(index), list_menu_width);
		if (index == menu->view_sel) {
			wbkgd(w, get_reversed_color_pair(menu->paint_action(index)));
		} else {
			wbkgd(w, get_color_pair(menu->paint_action(index)));
		}
		wrefresh(w);
	}
}

void
expose_all_visible_entries_of_the_list_menu(void)
{
	for (size_t i = menu->view_min; i <= menu->view_max && i < menu->entries_count; ++i) {
		expose_entry_of_the_list_menu(i);
	}
}

static inline void
erase_all_visible_entries_not_in_the_list_menu(void)
{
	for (size_t i = menu->entries_count; i <= menu->view_max; ++i) {
		werase(windows[i]);
		wbkgd(windows[i], COLOR_PAIR(0));
		wnoutrefresh(windows[i]);
	}
	doupdate();
}

void
redraw_list_menu(void)
{
	if (list_menu_is_paused == false) {
		menu->view_max = menu->view_min + (list_menu_height - 1);
		if (menu->view_sel > menu->view_max) {
			menu->view_max = menu->view_sel;
			menu->view_min = menu->view_max - (list_menu_height - 1);
		}
		expose_all_visible_entries_of_the_list_menu();
		erase_all_visible_entries_not_in_the_list_menu();
	}
}

size_t *
enter_list_menu(int8_t menu_index, size_t new_entries_count)
{
	menus_immersion[++menus_immersion_depth] = menu_index;
	menu = &menus[menu_index];
	menu->entries_count = new_entries_count;
	menu->view_sel = 0;
	menu->view_min = 0;
	redraw_list_menu();
	status_clean();
	return &(menu->view_sel);
}

void
leave_list_menu(void)
{
	menu = &menus[menus_immersion[--menus_immersion_depth]];
	if (menus_immersion[menus_immersion_depth] == SECTIONS_MENU) {
		refresh_unread_items_count_of_all_sections();
	}
	redraw_list_menu();
	status_clean();
}

void
pause_list_menu(void)
{
	list_menu_is_paused = true;
	status_clean();
}

void
resume_list_menu(void)
{
	list_menu_is_paused = false;
	redraw_list_menu();
	status_clean();
}

static void
list_menu_change_view(size_t new_sel)
{
	if (new_sel >= menu->entries_count) {
		if (menu->entries_count == 0) {
			return;
		}
		new_sel = menu->entries_count - 1;
	}

	if (new_sel > menu->view_max) {
		menu->view_min = new_sel - (list_menu_height - 1);
		menu->view_max = new_sel;
		menu->view_sel = new_sel;
		expose_all_visible_entries_of_the_list_menu();
	} else if (new_sel < menu->view_min) {
		menu->view_min = new_sel;
		menu->view_max = new_sel + (list_menu_height - 1);
		menu->view_sel = new_sel;
		expose_all_visible_entries_of_the_list_menu();
	} else if (new_sel != menu->view_sel) {
		WINDOW *w = windows[menu->view_sel - menu->view_min];
		wbkgd(w, get_color_pair(menu->paint_action(menu->view_sel)));
		wrefresh(w);
		menu->view_sel = new_sel;
		w = windows[menu->view_sel - menu->view_min];
		wbkgd(w, get_reversed_color_pair(menu->paint_action(menu->view_sel)));
		wrefresh(w);
	}

	if (menu->hover_action != NULL) {
		menu->hover_action();
	}
}

bool
handle_list_menu_navigation(input_cmd_id cmd)
{
	switch (cmd) {
		case INPUT_SELECT_NEXT:
			list_menu_change_view(menu->view_sel + 1);
			return true;
		case INPUT_SELECT_PREV:
			list_menu_change_view(menu->view_sel > 1 ? (menu->view_sel - 1) : 0);
			return true;
		case INPUT_SELECT_NEXT_UNREAD:
			for (size_t i = menu->view_sel + 1; i < menu->entries_count; ++i) {
				if (menu->unread_state(i) == true) {
					list_menu_change_view(i);
					break;
				}
			}
			return true;
		case INPUT_SELECT_PREV_UNREAD:
			for (int64_t i = (int64_t)menu->view_sel - 1; i >= 0; --i) {
				if (menu->unread_state(i) == true) {
					list_menu_change_view(i);
					break;
				}
			}
			return true;
		case INPUT_SELECT_NEXT_PAGE:
			list_menu_change_view(menu->view_sel + list_menu_height);
			return true;
		case INPUT_SELECT_PREV_PAGE:
			list_menu_change_view(menu->view_sel > list_menu_height ? (menu->view_sel - list_menu_height) : 0);
			return true;
		case INPUT_SELECT_FIRST:
			list_menu_change_view(0);
			return true;
		case INPUT_SELECT_LAST:
			list_menu_change_view(menu->entries_count > 1 ? (menu->entries_count - 1) : 0);
			return true;
	}
	return false;
}
