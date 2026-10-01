#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

// Unfortunately this global state has to be used to provide context for qsort.
static struct feed_entry **feeds_full = NULL;
static size_t feeds_full_size = 0;
static sorting_method_t feeds_sort = SORT_BY_INITIAL_ASC;

static bool
is_feed_valid(struct menu_state *ctx, size_t index)
{
	return index < ctx->feeds_view_size ? true : false;
}

static struct format_arg *
get_feed_args(struct menu_state *ctx, size_t index, struct format_arg *args, size_t args_size)
{
	if (args_size < 6) {
		return NULL;
	}
	struct feed_entry *feed = ctx->feeds_view[index];
	args[0] = (struct format_arg){L'i',  L'd',  {.i = index + 1}};
	args[1] = (struct format_arg){L'u',  L'd',  {.i = feed->unread_count}};
	args[2] = (struct format_arg){L'n',  L'd',  {.i = feed->items_count}};
	args[3] = (struct format_arg){L'l',  L's',  {.s = feed->url->ptr}};
	args[4] = (struct format_arg){L't',  L's',  {.s = STRING_IS_EMPTY(feed->name) ? feed->url->ptr : feed->name->ptr}};
	args[5] = (struct format_arg){L'\0', L'\0', {/* terminator */}};
	return args;
}

static struct config_color
paint_feed(struct menu_state *ctx, size_t index, bool is_selected)
{
	struct config_context **cfg = &ctx->feeds_view[index]->cfg;
	struct config_color color;
	if (ctx->feeds_view[index]->errors->len > 0 && !get_cfg_bool(&ctx->feeds_view[index]->cfg, CFG_SUPPRESS_ERRORS)) {
		color = get_cfg_color(cfg, CFG_COLOR_LIST_FEED_FAILED);
	} else if (ctx->feeds_view[index]->unread_count > 0) {
		color = get_cfg_color(cfg, CFG_COLOR_LIST_FEED_UNREAD);
	} else {
		color = get_cfg_color(cfg, CFG_COLOR_LIST_FEED);
	}
	if (is_selected) {
		if (is_cfg_color_set(cfg, CFG_COLOR_LIST_FEED_SELECTED)) {
			color = get_cfg_color(cfg, CFG_COLOR_LIST_FEED_SELECTED);
		} else {
			color.attributes |= TB_REVERSE;
		}
	}
	return color;
}

static bool
is_feed_unread(struct menu_state *ctx, size_t index)
{
	return ctx->feeds_view[index]->unread_count > 0;
}

static bool
is_feed_failed(struct menu_state *ctx, size_t index)
{
	return ctx->feeds_view[index]->errors->len > 0;
}

static int
compare_feeds_initial(const void *data1, const void *data2)
{
	struct feed_entry *feed1 = *(struct feed_entry **)data1;
	struct feed_entry *feed2 = *(struct feed_entry **)data2;
	size_t index1 = 0, index2 = 0;
	for (size_t i = 0; i < feeds_full_size; ++i) {
		if (feed1 == feeds_full[i]) index1 = i;
		if (feed2 == feeds_full[i]) index2 = i;
	}
	if (index1 > index2) return feeds_sort & 1 ? -1 : 1;
	if (index1 < index2) return feeds_sort & 1 ? 1 : -1;
	return 0;
}

static int
compare_feeds_unread(const void *data1, const void *data2)
{
	struct feed_entry *feed1 = *(struct feed_entry **)data1;
	struct feed_entry *feed2 = *(struct feed_entry **)data2;
	if (feed1->unread_count > feed2->unread_count) return feeds_sort & 1 ? -1 : 1;
	if (feed1->unread_count < feed2->unread_count) return feeds_sort & 1 ? 1 : -1;
	return compare_feeds_initial(data1, data2) * (feeds_sort & 1 ? -1 : 1);
}

static int
compare_feeds_alphabet(const void *data1, const void *data2)
{
	struct feed_entry *feed1 = *(struct feed_entry **)data1;
	struct feed_entry *feed2 = *(struct feed_entry **)data2;
	const char *token1 = STRING_IS_EMPTY(feed1->name) ? feed1->url->ptr : feed1->name->ptr;
	const char *token2 = STRING_IS_EMPTY(feed2->name) ? feed2->url->ptr : feed2->name->ptr;
	return strcmp(token1, token2) * (feeds_sort & 1 ? -1 : 1);
}

static inline void
filter_feeds(struct menu_state *m)
{
	if (hide_read_feeds) {
		size_t j = 0;
		for (size_t i = 0; i < m->feeds_full_size; ++i) {
			if (m->feeds_full[i]->unread_count > 0) {
				m->feeds_view[j++] = m->feeds_full[i];
			}
		}
		if (j == 0) {
			m->feeds_view_size = m->feeds_full_size;
			memcpy(m->feeds_view, m->feeds_full, sizeof(*m->feeds_view) * m->feeds_full_size);
		} else {
			m->feeds_view_size = j;
		}
	} else {
		m->feeds_view_size = m->feeds_full_size;
		memcpy(m->feeds_view, m->feeds_full, sizeof(*m->feeds_view) * m->feeds_full_size);
	}
}

static inline void
sort_feeds(struct menu_state *m, sorting_method_t method, bool need_redraw)
{
	feeds_full      = m->feeds_full;
	feeds_full_size = m->feeds_full_size;
	feeds_sort      = method;
	switch (feeds_sort & ~1) {
		case SORT_BY_UNREAD_ASC:   qsort(m->feeds_view, m->feeds_view_size, sizeof(*m->feeds_view), &compare_feeds_unread);   break;
		case SORT_BY_INITIAL_ASC:  qsort(m->feeds_view, m->feeds_view_size, sizeof(*m->feeds_view), &compare_feeds_initial);  break;
		case SORT_BY_ALPHABET_ASC: qsort(m->feeds_view, m->feeds_view_size, sizeof(*m->feeds_view), &compare_feeds_alphabet); break;
	}
	if (need_redraw) {
		expose_all_visible_entries_of_the_list_menu_unprotected();
	}
}

static inline void
rebuild_feeds(struct menu_state *m, sorting_method_t sort, struct feed_entry *selected_feed, bool need_redraw)
{
	pthread_mutex_lock(&interface_lock);
	filter_feeds(m);
	sort_feeds(m, sort, false);
	if (selected_feed) {
		size_t new_sel = 0;
		for (size_t i = 0; i < m->feeds_view_size; ++i) {
			if (m->feeds_view[i] == selected_feed) {
				new_sel = i;
				break;
			}
		}
		m->view_sel = new_sel;
	}
	if (need_redraw) {
		reset_list_menu_unprotected();
	}
	pthread_mutex_unlock(&interface_lock);
}

static void
feeds_refresh(struct menu_state *m)
{
	rebuild_feeds(m, feeds_sort, m->feeds_view[m->view_sel], true);
}

struct menu_state *
feeds_menu_loop(struct menu_state *m)
{
	m->enumerator   = &is_feed_valid;
	m->printer      = &list_menu_writer;
	m->get_args     = &get_feed_args;
	m->paint_action = &paint_feed;
	m->unread_state = &is_feed_unread;
	m->failed_state = &is_feed_failed;
	m->age_catch_up = get_cfg_bool(NULL, CFG_MENU_RESPONSIVENESS) ? feeds_refresh : NULL;
	m->entry_format = get_cfg_wstring(NULL, CFG_MENU_FEED_ENTRY_FORMAT);
	if (m->feeds_full_size < 1) {
		info_status("There are no feeds in this section");
		return close_menu();
	} else if (!(m->flags & MENU_DISABLE_SETTINGS)) {
		// Don't set the menu names here because it's redundant!
		if (get_cfg_bool(NULL, CFG_FEEDS_MENU_PARAMOUNT_EXPLORE) && db_count_items(m->feeds_full, m->feeds_full_size, false)) {
			return setup_menu(&items_menu_loop, NULL, m->feeds_full, m->feeds_full_size, MENU_IS_EXPLORE, NULL);
		} else if (m->feeds_full_size == 1 && db_count_items(m->feeds_full, m->feeds_full_size, false)) {
			return setup_menu(&items_menu_loop, NULL, m->feeds_full, m->feeds_full_size, MENU_SWALLOW, NULL);
		}
	}
	if (!m->is_initialized) {
		m->feeds_view = newsraft_realloc(m->feeds_view, sizeof(*m->feeds_view) * m->feeds_full_size);
		m->feeds_view_size = m->feeds_full_size;
		memcpy(m->feeds_view, m->feeds_full, sizeof(*m->feeds_view) * m->feeds_full_size);
		rebuild_feeds(m, get_sorting_id(get_cfg_string(NULL, CFG_MENU_FEED_SORTING)->ptr), NULL, false);
	}
	start_menu();
	input_id cmd;
	const struct wstring *arg;
	while (menu_read(m->feeds_view[m->view_sel]->binds, &cmd, NULL, &arg)) {
		if (handle_list_menu_control(m, cmd, arg)) {
			continue;
		}
		switch (cmd) {
			case INPUT_MARK_READ:       mark_feeds_read(m->feeds_view + m->view_sel, 1, true);     break;
			case INPUT_MARK_UNREAD:     mark_feeds_read(m->feeds_view + m->view_sel, 1, false);    break;
			case INPUT_MARK_READ_ALL:   mark_feeds_read(m->feeds_view, m->feeds_view_size, true);  break;
			case INPUT_MARK_UNREAD_ALL: mark_feeds_read(m->feeds_view, m->feeds_view_size, false); break;
			case INPUT_RELOAD:          queue_updates(m->feeds_view + m->view_sel, 1);             break;
			case INPUT_RELOAD_ALL:      queue_updates(m->feeds_full, m->feeds_full_size);          break;
			case INPUT_QUIT_HARD:       return NULL;
			case INPUT_ENTER:
				return setup_menu(&items_menu_loop, NULL, m->feeds_view + m->view_sel, 1, MENU_NORMAL, NULL);
			case INPUT_TOGGLE_EXPLORE_MODE:
				return setup_menu(&items_menu_loop, NULL, m->feeds_full, m->feeds_full_size, MENU_IS_EXPLORE, NULL);
			case INPUT_TOGGLE_READ_MENU:
			case INPUT_TOGGLE_READ_FEEDS: {
				hide_read_feeds = !hide_read_feeds;
				rebuild_feeds(m, feeds_sort, m->feeds_view[m->view_sel], true);
				break;
			}
			case INPUT_TOGGLE_READ_ITEMS:
				hide_read_items = !hide_read_items;
				break;
			case INPUT_APPLY_SEARCH_MODE_FILTER:
				return setup_menu(&items_menu_loop, NULL, m->feeds_view, m->feeds_view_size, MENU_IS_SEARCH | MENU_IS_EXPLORE, NULL);
			case INPUT_NAVIGATE_BACK:
				if (get_menu_depth() < 2) break;
				// fall through
			case INPUT_QUIT_SOFT:
				return close_menu();
			case INPUT_SORT_BY_UNREAD:
				pthread_mutex_lock(&interface_lock);
				sort_feeds(m, feeds_sort == SORT_BY_UNREAD_DESC ? SORT_BY_UNREAD_ASC : SORT_BY_UNREAD_DESC, true);
				pthread_mutex_unlock(&interface_lock);
				info_status(get_sorting_message(feeds_sort), "feeds");
				break;
			case INPUT_SORT_BY_INITIAL:
				pthread_mutex_lock(&interface_lock);
				sort_feeds(m, feeds_sort == SORT_BY_INITIAL_ASC ? SORT_BY_INITIAL_DESC : SORT_BY_INITIAL_ASC, true);
				pthread_mutex_unlock(&interface_lock);
				info_status(get_sorting_message(feeds_sort), "feeds");
				break;
			case INPUT_SORT_BY_ALPHABET:
				pthread_mutex_lock(&interface_lock);
				sort_feeds(m, feeds_sort == SORT_BY_ALPHABET_ASC ? SORT_BY_ALPHABET_DESC : SORT_BY_ALPHABET_ASC, true);
				pthread_mutex_unlock(&interface_lock);
				info_status(get_sorting_message(feeds_sort), "feeds");
				break;
			case INPUT_FIND_COMMAND:
				return setup_menu(&items_menu_loop, NULL, m->feeds_view, m->feeds_view_size, MENU_IS_EXPLORE, arg);
			case INPUT_DATABASE_COMMAND:
				db_perform_user_edit(arg, m->feeds_view + m->view_sel, 1, NULL);
				break;
			case INPUT_VIEW_ERRORS:
				return setup_menu(&errors_pager_loop, NULL, m->feeds_view + m->view_sel, 1, MENU_NORMAL, NULL);
		}
	}
	return NULL;
}
