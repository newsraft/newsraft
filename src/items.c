#include "newsraft.h"

static volatile size_t items_age = 0;

static bool
is_item_valid(struct menu_state *ctx, size_t index)
{
	if (ctx->items == NULL) {
		return false;
	}
	obtain_items_at_least_up_to_the_given_index(ctx->items, NULL, index);
	return index < ctx->items->len ? true : false;
}

static const struct format_arg *
get_item_args(struct menu_state *ctx, size_t index)
{
	static struct format_arg item_fmt[] = {
		{L'i',  L'd',  {.i = 0   }},
		{L'u',  L's',  {.s = NULL}},
		{L'd',  L's',  {.s = NULL}},
		{L'D',  L's',  {.s = NULL}},
		{L'l',  L's',  {.s = NULL}},
		{L't',  L's',  {.s = NULL}},
		{L'o',  L's',  {.s = NULL}},
		{L'L',  L's',  {.s = NULL}},
		{L'T',  L's',  {.s = NULL}},
		{L'O',  L's',  {.s = NULL}},
		{L'\0', L'\0', {.i = 0   }}, // terminator
	};
	item_fmt[0].value.i = index + 1;
	item_fmt[1].value.s = ctx->items->ptr[index].is_unread == true ? "N" : " ";
	item_fmt[2].value.s = ctx->items->ptr[index].date_str->ptr;
	item_fmt[3].value.s = ctx->items->ptr[index].pub_date_str->ptr;
	item_fmt[4].value.s = ctx->items->ptr[index].url->ptr;
	item_fmt[5].value.s = ctx->items->ptr[index].title->ptr;
	item_fmt[6].value.s = ctx->items->ptr[index].title->len > 0 ? ctx->items->ptr[index].title->ptr : ctx->items->ptr[index].url->ptr;
	item_fmt[7].value.s = ctx->items->ptr[index].feed[0]->url->ptr;
	item_fmt[8].value.s = ctx->items->ptr[index].feed[0]->name ? ctx->items->ptr[index].feed[0]->name->ptr : "";
	item_fmt[9].value.s = ctx->items->ptr[index].feed[0]->name ? ctx->items->ptr[index].feed[0]->name->ptr : ctx->items->ptr[index].feed[0]->url->ptr;
	return item_fmt;
}

static struct config_color
paint_item(struct menu_state *ctx, size_t index)
{
	struct config_context **cfg = &ctx->items->ptr[index].feed[0]->cfg;

	if (ctx->items->ptr[index].is_important == true) {
		return get_cfg_color(cfg, CFG_COLOR_LIST_ITEM_IMPORTANT);
	} else if (ctx->items->ptr[index].is_unread == true) {
		return get_cfg_color(cfg, CFG_COLOR_LIST_ITEM_UNREAD);
	} else {
		return get_cfg_color(cfg, CFG_COLOR_LIST_ITEM);
	}
}

static bool
is_item_unread(struct menu_state *ctx, size_t index)
{
	return ctx->items->ptr[index].is_unread;
}

bool
important_item_condition(struct menu_state *ctx, size_t index)
{
	return ctx->items->ptr[index].is_important;
}

struct string *
generate_items_search_condition(struct feed_entry **feeds, size_t feeds_count)
{
	if (feeds_count == 0) {
		return NULL;
	}
	struct string *cond = crtas("(", 1);
	for (size_t i = 0; i < feeds_count; ++i) {
		if (i > 0) {
			catas(cond, " OR ", 4);
		}
		catas(cond, "feed_url=?", 10);
		const struct string *rule = get_cfg_string(&feeds[i]->cfg, CFG_ITEM_RULE);
		if (!STRING_IS_EMPTY(rule)) {
			catas(cond, " AND (", 6);
			catss(cond, rule);
			catcs(cond, ')');
		}
	}
	catcs(cond, ')');
	return cond;
}

static void
mark_item_read(struct menu_state *ctx, size_t view_sel, bool status)
{
	if (ctx->items->ptr[view_sel].is_unread == status) {
		if (db_mark_item_read(ctx->items->ptr[view_sel].rowid, status) == true) {
			ctx->items->ptr[view_sel].is_unread = !status;
			expose_entry_of_the_list_menu(view_sel);
		}
	}
}

static void
mark_item_important(struct menu_state *ctx, size_t view_sel, bool status)
{
	if (ctx->items->ptr[view_sel].is_important != status) {
		if (db_mark_item_important(ctx->items->ptr[view_sel].rowid, status) == true) {
			ctx->items->ptr[view_sel].is_important = status;
			expose_entry_of_the_list_menu(view_sel);
		}
	}
}

static void
toggle_item_read(struct menu_state *ctx, size_t view_sel)
{
	mark_item_read(ctx, view_sel, ctx->items->ptr[view_sel].is_unread);
}

static void
toggle_item_important(struct menu_state *ctx, size_t view_sel)
{
	mark_item_important(ctx, view_sel, !ctx->items->ptr[view_sel].is_important);
}

static void
mark_all_items_read(struct menu_state *ctx, bool status)
{
	pthread_mutex_lock(&interface_lock);
	if (ctx->flags & MENU_IS_SEARCH) {
		for (size_t i = 0; i < ctx->items->len; ++i) {
			if (db_mark_item_read(ctx->items->ptr[i].rowid, status) == true) {
				ctx->items->ptr[i].is_unread = !status;
			}
		}
		pthread_mutex_unlock(&interface_lock);
	} else {
		// Use intermediate variables to avoid race condition
		struct feed_entry **items_feeds = ctx->items->feeds;
		size_t items_feeds_count = ctx->items->feeds_count;
		pthread_mutex_unlock(&interface_lock);
		mark_feeds_read(items_feeds, items_feeds_count, status);
	}
	expose_all_visible_entries_of_the_list_menu();
}

void
tell_items_menu_to_regenerate(void)
{
	items_age += 1;
	break_getting_input_command();
}

struct menu_state *
items_menu_loop(struct menu_state *m)
{
	m->enumerator   = &is_item_valid;
	m->printer      = &list_menu_writer;
	m->get_args     = &get_item_args;
	m->paint_action = &paint_item;
	m->unread_state = &is_item_unread;
	m->entry_format = get_cfg_wstring(NULL, m->flags & MENU_IS_EXPLORE ? CFG_MENU_EXPLORE_ITEM_ENTRY_FORMAT : CFG_MENU_ITEM_ENTRY_FORMAT);
	items_age += 1;
	if (m->is_initialized == false) {
		m->items_age = items_age;
		if (!update_menu_item_list(m)) {
			return close_menu(); // Error displayed by update_menu_item_list()
		}
	}
	start_menu();
	const struct wstring *arg, *browser;
	while (true) {
		if (get_cfg_bool(NULL, CFG_MENU_RESPONSIVENESS) && m->items_age != items_age) {
			m->items_age = items_age;
			update_menu_item_list(m);
		}
		if (get_cfg_bool(&m->items->ptr[m->view_sel].feed[0]->cfg, CFG_MARK_ITEM_READ_ON_HOVER)) {
			mark_item_read(m, m->view_sel, true);
		}
		input_id cmd = get_input(m->items->ptr[m->view_sel].feed[0]->binds, NULL, &arg);
		if (handle_list_menu_control(m, cmd, arg) == true) {
			continue;
		}
		switch (cmd) {
			case INPUT_MARK_READ:         mark_item_read(m, m->view_sel, true);                     break;
			case INPUT_MARK_UNREAD:       mark_item_read(m, m->view_sel, false);                    break;
			case INPUT_TOGGLE_READ:       toggle_item_read(m, m->view_sel);                         break;
			case INPUT_MARK_READ_ALL:     mark_all_items_read(m, true);                             break;
			case INPUT_MARK_UNREAD_ALL:   mark_all_items_read(m, false);                            break;
			case INPUT_MARK_IMPORTANT:    mark_item_important(m, m->view_sel, true);                break;
			case INPUT_MARK_UNIMPORTANT:  mark_item_important(m, m->view_sel, false);               break;
			case INPUT_TOGGLE_IMPORTANT:  toggle_item_important(m, m->view_sel);                    break;
			case INPUT_RELOAD:            queue_updates(m->items->ptr[m->view_sel].feed, 1);        break;
			case INPUT_RELOAD_ALL:        queue_updates(m->feeds_original, m->feeds_count);         break;
			case INPUT_COPY_TO_CLIPBOARD: copy_string_to_clipboard(m->items->ptr[m->view_sel].url); break;
			case INPUT_QUIT_HARD:         return NULL;
			case INPUT_NAVIGATE_BACK:
				if (get_menu_depth() < 3 && (!(m->flags & MENU_IS_SEARCH) && (m->flags & MENU_IS_EXPLORE) && (m->find_filter == NULL)))
				{
				  break;
				}
				// fall through
			case INPUT_QUIT_SOFT:
				if (!(m->flags & MENU_IS_SEARCH) && (m->flags & MENU_IS_EXPLORE) && (m->find_filter == NULL)) {
					close_menu();
				}
				return close_menu();
			case INPUT_TOGGLE_EXPLORE_MODE:
				if (m->flags & MENU_IS_EXPLORE) return close_menu();
				break;
			case INPUT_GOTO_FEED:
				if (!(m->flags & MENU_IS_EXPLORE)) break;
				return setup_menu(&items_menu_loop, NULL, m->items->ptr[m->view_sel].feed, 1, MENU_NORMAL, NULL);
			case INPUT_APPLY_SEARCH_MODE_FILTER:
				return setup_menu(&items_menu_loop, NULL, m->feeds_original, m->feeds_count, MENU_IS_SEARCH | MENU_IS_EXPLORE, m->find_filter);
			case INPUT_OPEN_IN_BROWSER:
				browser = get_cfg_wstring(&m->items->ptr[m->view_sel].feed[0]->cfg, CFG_OPEN_IN_BROWSER_COMMAND);
				run_formatted_command(browser, get_item_args(m, m->view_sel));
				break;
			case INPUT_SORT_BY_TIME:
			case INPUT_SORT_BY_TIME_DOWNLOAD:
			case INPUT_SORT_BY_TIME_PUBLICATION:
			case INPUT_SORT_BY_TIME_UPDATE:
			case INPUT_SORT_BY_ROWID:
			case INPUT_SORT_BY_UNREAD:
			case INPUT_SORT_BY_ALPHABET:
			case INPUT_SORT_BY_IMPORTANT:
				change_items_list_sorting(m, cmd); break;
			case INPUT_FIND_COMMAND:
				if (m->find_filter) return setup_menu(&items_menu_loop, NULL, m->feeds_original, m->feeds_count, MENU_IS_EXPLORE | MENU_SWALLOW, arg);;
				return setup_menu(&items_menu_loop, NULL, m->feeds_original, m->feeds_count, MENU_IS_EXPLORE, arg);
			case INPUT_DATABASE_COMMAND:
				db_perform_user_edit(arg, NULL, 0, &m->items->ptr[m->view_sel]);
				break;
			case INPUT_ENTER:
				return setup_menu(&item_pager_loop, m->items->ptr[m->view_sel].title, NULL, 0, MENU_NORMAL, NULL);
		}
	}
	return close_menu();
}
