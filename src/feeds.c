#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

static struct feed_entry **feeds = NULL;
static size_t feeds_count = 0;
static sorting_method_t feeds_sort = SORT_BY_INITIAL_ASC;
static struct feed_entry **feeds_original = NULL;

static bool
is_feed_valid(struct menu_state *ctx, size_t index)
{
	return index < ctx->feeds_count ? true : false;
}

static const struct format_arg *
get_feed_args(struct menu_state *ctx, size_t index)
{
	static struct format_arg feed_fmt[] = {
		{L'i',  L'd',  {.i = 0   }},
		{L'u',  L'd',  {.i = 0   }},
		{L'l',  L's',  {.s = NULL}},
		{L't',  L's',  {.s = NULL}},
		{L'\0', L'\0', {.i = 0   }}, // terminator
	};
	feed_fmt[0].value.i = index + 1;
	feed_fmt[1].value.i = ctx->feeds[index]->unread_count;
	feed_fmt[2].value.s = ctx->feeds[index]->url->ptr;
	feed_fmt[3].value.s = STRING_IS_EMPTY(ctx->feeds[index]->name) ? ctx->feeds[index]->url->ptr : ctx->feeds[index]->name->ptr;
	return feed_fmt;
}

static struct config_color
paint_feed(struct menu_state *ctx, size_t index, bool is_selected)
{
	struct config_context **cfg = &ctx->feeds[index]->cfg;
	struct config_color color;
	if (ctx->feeds[index]->errors->len > 0 && !get_cfg_bool(&ctx->feeds[index]->cfg, CFG_SUPPRESS_ERRORS)) {
		color = get_cfg_color(cfg, CFG_COLOR_LIST_FEED_FAILED);
	} else if (ctx->feeds[index]->unread_count > 0) {
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
	return ctx->feeds[index]->unread_count > 0;
}

static bool
is_feed_failed(struct menu_state *ctx, size_t index)
{
	return ctx->feeds[index]->errors->len > 0;
}

static int
compare_feeds_initial(const void *data1, const void *data2)
{
	struct feed_entry *feed1 = *(struct feed_entry **)data1;
	struct feed_entry *feed2 = *(struct feed_entry **)data2;
	size_t index1 = 0, index2 = 0;
	for (size_t i = 0; i < feeds_count; ++i) {
		if (feed1 == feeds_original[i]) index1 = i;
		if (feed2 == feeds_original[i]) index2 = i;
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
sort_feeds(struct menu_state *m, sorting_method_t method, bool we_are_already_in_feeds_menu)
{
	pthread_mutex_lock(&interface_lock);
	feeds          = m->feeds;
	feeds_count    = m->feeds_count;
	feeds_original = m->feeds_original;
	feeds_sort     = method;
	switch (feeds_sort & ~1) {
		case SORT_BY_UNREAD_ASC:   qsort(feeds, feeds_count, sizeof(struct feed_entry *), &compare_feeds_unread);   break;
		case SORT_BY_INITIAL_ASC:  qsort(feeds, feeds_count, sizeof(struct feed_entry *), &compare_feeds_initial);  break;
		case SORT_BY_ALPHABET_ASC: qsort(feeds, feeds_count, sizeof(struct feed_entry *), &compare_feeds_alphabet); break;
	}
	pthread_mutex_unlock(&interface_lock);
	if (we_are_already_in_feeds_menu == true) {
		expose_all_visible_entries_of_the_list_menu();
		info_status(get_sorting_message(feeds_sort), "feeds");
	}
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
	m->entry_format = get_cfg_wstring(NULL, CFG_MENU_FEED_ENTRY_FORMAT);
	if (m->feeds_count < 1) {
		info_status("There are no feeds in this section");
		return close_menu();
	} else if (!(m->flags & MENU_DISABLE_SETTINGS)) {
		// Don't set the menu names here because it's redundant!
		if (get_cfg_bool(NULL, CFG_FEEDS_MENU_PARAMOUNT_EXPLORE) && db_count_items(m->feeds_original, m->feeds_count, false)) {
			return setup_menu(&items_menu_loop, NULL, m->feeds_original, m->feeds_count, MENU_IS_EXPLORE, NULL);
		} else if (m->feeds_count == 1 && db_count_items(m->feeds_original, m->feeds_count, false)) {
			return setup_menu(&items_menu_loop, NULL, m->feeds_original, m->feeds_count, MENU_SWALLOW, NULL);
		}
	}
	if (m->is_initialized == false) {
		m->feeds = newsraft_malloc(sizeof(struct feed_entry *) * m->feeds_count);
		memcpy(m->feeds, m->feeds_original, sizeof(struct feed_entry *) * m->feeds_count);
		sort_feeds(m, get_sorting_id(get_cfg_string(NULL, CFG_MENU_FEED_SORTING)->ptr), false);
	}
	start_menu();
	const struct wstring *arg;
	while (true) {
		input_id cmd = get_input(m->feeds[m->view_sel]->binds, NULL, &arg);
		if (handle_list_menu_control(m, cmd, arg) == true) {
			continue;
		}
		switch (cmd) {
			case INPUT_MARK_READ:       mark_feeds_read(m->feeds + m->view_sel, 1, true);  break;
			case INPUT_MARK_UNREAD:     mark_feeds_read(m->feeds + m->view_sel, 1, false); break;
			case INPUT_MARK_READ_ALL:   mark_feeds_read(m->feeds, m->feeds_count, true);   break;
			case INPUT_MARK_UNREAD_ALL: mark_feeds_read(m->feeds, m->feeds_count, false);  break;
			case INPUT_RELOAD:          queue_updates(m->feeds + m->view_sel, 1);          break;
			case INPUT_RELOAD_ALL:      queue_updates(m->feeds_original, m->feeds_count);  break;
			case INPUT_QUIT_HARD:       return NULL;
			case INPUT_ENTER:
				return setup_menu(&items_menu_loop, NULL, m->feeds + m->view_sel, 1, MENU_NORMAL, NULL);
			case INPUT_TOGGLE_EXPLORE_MODE:
				return setup_menu(&items_menu_loop, NULL, m->feeds, m->feeds_count, MENU_IS_EXPLORE, NULL);
			case INPUT_APPLY_SEARCH_MODE_FILTER:
				return setup_menu(&items_menu_loop, NULL, m->feeds, m->feeds_count, MENU_IS_SEARCH | MENU_IS_EXPLORE, NULL);
			case INPUT_NAVIGATE_BACK:
				if (get_menu_depth() < 2) break;
				// fall through
			case INPUT_QUIT_SOFT:
				return close_menu();
			case INPUT_SORT_BY_UNREAD:
				sort_feeds(m, feeds_sort == SORT_BY_UNREAD_DESC ? SORT_BY_UNREAD_ASC : SORT_BY_UNREAD_DESC, true);
				break;
			case INPUT_SORT_BY_INITIAL:
				sort_feeds(m, feeds_sort == SORT_BY_INITIAL_ASC ? SORT_BY_INITIAL_DESC : SORT_BY_INITIAL_ASC, true);
				break;
			case INPUT_SORT_BY_ALPHABET:
				sort_feeds(m, feeds_sort == SORT_BY_ALPHABET_ASC ? SORT_BY_ALPHABET_DESC : SORT_BY_ALPHABET_ASC, true);
				break;
			case INPUT_FIND_COMMAND:
				return setup_menu(&items_menu_loop, NULL, m->feeds_original, m->feeds_count, MENU_IS_EXPLORE, arg);
			case INPUT_DATABASE_COMMAND:
				db_perform_user_edit(arg, m->feeds + m->view_sel, 1, NULL);
				break;
			case INPUT_VIEW_ERRORS:
				return setup_menu(&errors_pager_loop, NULL, m->feeds + m->view_sel, 1, MENU_NORMAL, NULL);
		}
	}
	return NULL;
}
