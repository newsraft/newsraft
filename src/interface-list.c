#include <assert.h>
#include <stdlib.h>
#include "newsraft.h"

// Note to the future.
// TODO: explain why we need all these *_unprotected functions

size_t list_menu_height;
size_t list_menu_width;

static WINDOW **windows = NULL;
static size_t windows_count = 0;
static size_t scrolloff;
static size_t horizontal_shift = 0;
static struct wstring *list_fmtout = NULL;

static struct menu_state *menus = NULL;
static struct menu_state *menu  = NULL;

bool
is_current_menu_a_pager(void)
{
	return menu != NULL && menu->enumerator == &is_pager_pos_valid ? true : false;
}

bool
adjust_list_menu(void)
{
	INFO("Adjusting list menu.");
	// Delete old windows to create new windows with the new size.
	for (size_t i = 0; i < windows_count; ++i) {
		delwin(windows[i]);
	}
	windows = newsraft_realloc(windows, sizeof(WINDOW *) * list_menu_height);
	for (size_t i = 0; i < list_menu_height; ++i) {
		windows[i] = newwin(i);
		if (windows[i] == NULL) {
			windows_count = i;
			FAIL("newwin() returned NULL!");
			return false;
		}
	}
	windows_count = list_menu_height;
	scrolloff = get_cfg_uint(NULL, CFG_SCROLLOFF);
	if (scrolloff > (list_menu_height / 2)) {
		scrolloff = list_menu_height / 2;
	}
	wstr_set(&list_fmtout, NULL, 0, 200);
	return true;
}

void
free_list_menu(void)
{
	for (size_t i = 0; i < windows_count; ++i) {
		delwin(windows[i]);
	}
	newsraft_free(windows);
	free_wstring(list_fmtout);
}

void
list_menu_writer(size_t index, WINDOW *w)
{
	if (menu->enumerator(menu, index) == true) {
		do_format(list_fmtout, menu->entry_format->ptr, menu->get_args(menu, index));
		if (list_fmtout->len > horizontal_shift) {
			waddwstr(w, list_fmtout->ptr + horizontal_shift);
		}
		struct config_color color = menu->paint_action(menu, index);
		if (index == menu->view_sel) {
			color.attributes |= TB_REVERSE;
		}
		wbkgd(w, color);
	}
}

static inline void
expose_entry_of_the_list_menu_unprotected(size_t index)
{
	WINDOW *w = windows[index - menu->view_min];
	werase(w);
	wmove(w, 0);
	wbkgd(w, (struct config_color){TB_DEFAULT, TB_DEFAULT, TB_DEFAULT});
	wattrset(w, TB_DEFAULT);
	menu->printer(index, w);
}

void
expose_entry_of_the_list_menu(size_t index)
{
	pthread_mutex_lock(&interface_lock);
	if (index >= menu->view_min && index <= menu->view_max) {
		expose_entry_of_the_list_menu_unprotected(index);
		tb_present();
	}
	pthread_mutex_unlock(&interface_lock);
}

static inline void
expose_all_visible_entries_of_the_list_menu_unprotected(void)
{
	for (size_t i = menu->view_min; i <= menu->view_max; ++i) {
		expose_entry_of_the_list_menu_unprotected(i);
	}
	tb_present();
	update_status_window_content_unprotected();
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
	while (menu->view_max >= list_menu_height && menu->enumerator(menu, menu->view_max) == false) {
		menu->view_max -= 1;
	}
	menu->view_min = menu->view_max - (list_menu_height - 1);
	expose_all_visible_entries_of_the_list_menu_unprotected();
}

void
reset_list_menu_unprotected(void)
{
	if (menu->enumerator(menu, menu->view_sel) == false) {
		menu->view_sel = 0;
		menu->view_min = 0;
	}
	redraw_list_menu_unprotected();
}

static size_t
obtain_list_entries_count_unprotected(struct menu_state *m)
{
	size_t i = 0;
	while (m->enumerator(m, i) == true) {
		i += 1;
	}
	return i;
}

static void
change_list_view_unprotected(struct menu_state *m, size_t new_sel, bool is_wrappable)
{
	if (is_wrappable && get_cfg_bool(NULL, CFG_SCROLLWRAP)) {
		if (new_sel > m->view_sel) {
			// Tried to go forward
			if (m->enumerator(m, m->view_sel) && !m->enumerator(m, m->view_sel + 1)) {
				new_sel = 0;
			}
		} else if (new_sel == 0 && m->view_sel == 0) {
			// Tried to go backward
			new_sel = obtain_list_entries_count_unprotected(m);
		}
	}
	while (m->enumerator(m, new_sel) == false && new_sel > 0) {
		new_sel -= 1;
	}
	if (m->enumerator(m, new_sel) == false) {
		return;
	}
	if (new_sel + scrolloff > m->view_max) {
		m->view_max = new_sel + scrolloff;
		while (m->view_max >= list_menu_height && m->enumerator(m, m->view_max) == false) {
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
			wbkgd(windows[m->view_sel - m->view_min], m->paint_action(m, m->view_sel));
			m->view_sel = new_sel;
			struct config_color color = m->paint_action(m, m->view_sel);
			color.attributes |= TB_REVERSE;
			wbkgd(windows[m->view_sel - m->view_min], color);
			tb_present();
		} else {
			m->view_sel = new_sel;
		}
	}
}

static void
change_list_view_filtered_unprotected(struct menu_state *m, bool (*filter)(struct menu_state *, size_t), bool is_forward)
{
	if (!filter) {
		return;
	}
	if (is_forward) {
		for (size_t i = m->view_sel + 1, j = 0; j < 2; i = 0, ++j) {
			while (m->enumerator(m, i) == true) {
				if (filter(m, i) == true) {
					change_list_view_unprotected(m, i, false);
					j = 2;
					break;
				}
				i += 1;
			}
			if (!get_cfg_bool(NULL, CFG_SCROLLWRAP)) {
				break;
			}
		}
	} else {
		for (size_t i = m->view_sel, j = 0; j < 2; ++j) {
			while (i > 0 && m->enumerator(m, i - 1) == true) {
				if (filter(m, i - 1) == true) {
					change_list_view_unprotected(m, i - 1, false);
					j = 2;
					break;
				}
				i -= 1;
			}
			if (j < 2) {
				i = obtain_list_entries_count_unprotected(m);
			}
			if (!get_cfg_bool(NULL, CFG_SCROLLWRAP)) {
				break;
			}
		}
	}
}

bool
handle_list_menu_control(struct menu_state *m, input_id cmd, const struct wstring *arg)
{
	pthread_mutex_lock(&interface_lock);
	if (cmd == INPUT_SELECT_NEXT || cmd == INPUT_JUMP_TO_NEXT) {
		change_list_view_unprotected(m, m->view_sel + 1, true);
	} else if (cmd == INPUT_SELECT_PREV || cmd == INPUT_JUMP_TO_PREV) {
		change_list_view_unprotected(m, m->view_sel > 1 ? m->view_sel - 1 : 0, true);
	} else if (cmd == INPUT_JUMP_TO_NEXT_UNREAD) {
		change_list_view_filtered_unprotected(m, m->unread_state, true);
	} else if (cmd == INPUT_JUMP_TO_PREV_UNREAD) {
		change_list_view_filtered_unprotected(m, m->unread_state, false);
	} else if (cmd == INPUT_JUMP_TO_NEXT_IMPORTANT && m->run == &items_menu_loop) {
		change_list_view_filtered_unprotected(m, important_item_condition, true);
	} else if (cmd == INPUT_JUMP_TO_PREV_IMPORTANT && m->run == &items_menu_loop) {
		change_list_view_filtered_unprotected(m, important_item_condition, false);
	} else if (cmd == INPUT_JUMP_TO_NEXT_ERROR) {
		change_list_view_filtered_unprotected(m, m->failed_state, true);
	} else if (cmd == INPUT_JUMP_TO_PREV_ERROR) {
		change_list_view_filtered_unprotected(m, m->failed_state, false);
	} else if (cmd == INPUT_SELECT_NEXT_PAGE) {
		change_list_view_unprotected(m, m->view_sel + list_menu_height, true);
	} else if (cmd == INPUT_SELECT_NEXT_PAGE_HALF) {
		change_list_view_unprotected(m, m->view_sel + list_menu_height / 2, true);
	} else if (cmd == INPUT_SELECT_PREV_PAGE) {
		change_list_view_unprotected(m, m->view_sel > list_menu_height ? m->view_sel - list_menu_height : 0, true);
	} else if (cmd == INPUT_SELECT_PREV_PAGE_HALF) {
		size_t step = list_menu_height / 2;
		change_list_view_unprotected(m, m->view_sel > step ? m->view_sel - step : 0, true);
	} else if (cmd == INPUT_SELECT_FIRST) {
		change_list_view_unprotected(m, 0, false);
	} else if (cmd == INPUT_SELECT_LAST) {
		change_list_view_unprotected(m, obtain_list_entries_count_unprotected(m), false);
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
	} else if (cmd == INPUT_CLEAN_STATUS) {
		status_clean_unprotected();
	} else if (cmd == INPUT_SYSTEM_COMMAND) {
		pthread_mutex_unlock(&interface_lock);
		run_formatted_command(arg, m->get_args(m, m->view_sel));
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
	while (menu->enumerator(menu, new_sel) == false && new_sel > 0) {
		new_sel -= 1;
	}
	if (menu->enumerator(menu, new_sel) == false) {
		return;
	}
	size_t entries_count = obtain_list_entries_count_unprotected(menu);
	if (entries_count - new_sel < list_menu_height) {
		new_sel = entries_count > list_menu_height ? entries_count - list_menu_height : 0;
	}
	if (new_sel != menu->view_min) {
		menu->view_max = new_sel + (list_menu_height - 1);
		while (menu->view_max >= list_menu_height && menu->enumerator(menu, menu->view_max) == false) {
			menu->view_max -= 1;
		}
		menu->view_min = menu->view_max - (list_menu_height - 1);
		expose_all_visible_entries_of_the_list_menu_unprotected();
	}
}

bool
handle_pager_menu_control(input_id cmd)
{
	pthread_mutex_lock(&interface_lock);
	if (cmd == INPUT_SELECT_NEXT || cmd == INPUT_ENTER) {
		change_pager_view_unprotected(menu->view_min + 1);
	} else if (cmd == INPUT_SELECT_PREV) {
		change_pager_view_unprotected(menu->view_min > 0 ? menu->view_min - 1 : 0);
	} else if (cmd == INPUT_SELECT_NEXT_PAGE) {
		change_pager_view_unprotected(menu->view_min + list_menu_height);
	} else if (cmd == INPUT_SELECT_NEXT_PAGE_HALF) {
		change_pager_view_unprotected(menu->view_min + list_menu_height / 2);
	} else if (cmd == INPUT_SELECT_PREV_PAGE) {
		change_pager_view_unprotected(menu->view_min > list_menu_height ? menu->view_min - list_menu_height : 0);
	} else if (cmd == INPUT_SELECT_PREV_PAGE_HALF) {
		size_t step = list_menu_height / 2;
		change_pager_view_unprotected(menu->view_min > step ? menu->view_min - step : 0);
	} else if (cmd == INPUT_SELECT_FIRST) {
		change_pager_view_unprotected(0);
	} else if (cmd == INPUT_SELECT_LAST) {
		change_pager_view_unprotected(obtain_list_entries_count_unprotected(menu));
	} else if (cmd == INPUT_CLEAN_STATUS) {
		status_clean_unprotected();
	} else {
		pthread_mutex_unlock(&interface_lock);
		return false;
	}
	pthread_mutex_unlock(&interface_lock);
	return true;
}

static inline void
free_deleted_menus(void)
{
	while (menus != NULL && menus->is_deleted == true) {
		struct menu_state *tmp = menus;
		menus = menus->prev;
		free_string(tmp->name);
		free_items_list(tmp->items);
		newsraft_free(tmp->feeds);
		newsraft_free(tmp);
	}
}

void
free_menus(void)
{
	while (menus != NULL) {
		struct menu_state *tmp = menus;
		menus = menus->prev;
		free_string(tmp->name);
		free_items_list(tmp->items);
		newsraft_free(tmp->feeds);
		newsraft_free(tmp);
	}
}

size_t
get_menu_depth(void)
{
	size_t depth = 0;
	for (struct menu_state *i = menus; i != NULL && i->is_deleted == false; i = i->prev) {
		depth += 1;
	}
	return depth;
}

static void
update_unread_items_count_of_last_menu(void)
{
	if (menus != NULL && menus->feeds_original != NULL && menus->feeds_count > 0) {
		for (size_t i = 0; i < menus->feeds_count; ++i) {
			menus->feeds_original[i]->unread_count = db_count_items(&menus->feeds_original[i], 1, true);
		}
	}
}

struct menu_state *
setup_menu(struct menu_state *(*run)(struct menu_state *), const struct string *name, struct feed_entry **feeds, size_t feeds_count, uint32_t flags, const void *ctx)
{
	pthread_mutex_lock(&interface_lock);
	update_unread_items_count_of_last_menu();
	struct menu_state *new = newsraft_calloc(1, sizeof(struct menu_state));
	new->run            = run;
	new->feeds_original = feeds;
	new->feeds_count    = feeds_count;
	new->flags          = flags;
	new->prev           = menus;
	new->find_filter    = ctx;
	if (!STRING_IS_EMPTY(name)) {
		cpyss(&new->name, name);
	} else if (new->find_filter) {
		new->name = convert_wstring_to_string(new->find_filter);
	} else if (feeds != NULL && feeds_count == 1) {
		cpyss(&new->name, STRING_IS_EMPTY(feeds[0]->name) ? feeds[0]->url : feeds[0]->name);
	}
	if ((flags & MENU_SWALLOW) && menus != NULL) {
		menus->is_deleted = true;
	}
	menus = new;
	pthread_mutex_unlock(&interface_lock);
	return menus;
}

struct menu_state *
close_menu(void)
{
	pthread_mutex_lock(&interface_lock);
	update_unread_items_count_of_last_menu();
	struct menu_state *next_menu = NULL;
	for (struct menu_state *i = menus; i != NULL; i = i->prev) {
		if (i->is_deleted == false) {
			i->is_deleted = true;
			break;
		}
	}
	for (struct menu_state *i = menus; i != NULL; i = i->prev) {
		if (i->is_deleted == false) {
			next_menu = i;
			next_menu->flags |= MENU_DISABLE_SETTINGS;
			break;
		}
	}
	pthread_mutex_unlock(&interface_lock);
	return next_menu;
}

void
start_menu(void)
{
	pthread_mutex_lock(&interface_lock);
	free_deleted_menus();
	menu = menus;

	// These methods are mandatory for all menus.
	assert(menu->enumerator);
	assert(menu->printer);

	horizontal_shift = 0;
	if (menu->is_initialized == false) {
		menu->view_sel = 0;
		menu->view_min = 0;
		status_clean_unprotected();
	}
	redraw_list_menu_unprotected();
	menu->is_initialized = true;
	pthread_mutex_unlock(&interface_lock);
}

void
write_menu_path_string(struct string *names, struct menu_state *m)
{
	if (menus == NULL) {
		return;
	} else if (m == NULL) {
		write_menu_path_string(names, menus);
		return;
	} else if (m->prev != NULL) {
		write_menu_path_string(names, m->prev);
	}
	if (m->is_deleted == false && m->name != NULL && m->name->len > 0) {
		catas(names, "  >  ", 5);
		catss(names, m->name);
	}
}
