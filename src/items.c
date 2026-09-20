#include "newsraft.h"

static bool
is_item_valid(struct menu_state *ctx, size_t index)
{
	if (ctx->items == NULL) {
		return false;
	}
	obtain_items_at_least_up_to_the_given_index(ctx->items, index);
	return index < ctx->items->len ? true : false;
}

static struct format_arg *
get_item_args(struct menu_state *ctx, size_t index, struct format_arg *args, size_t args_size)
{
	if (args_size < 11) {
		return NULL;
	}
	struct item_entry *item = &ctx->items->ptr[index];
	args[0]  = (struct format_arg){L'i',  L'd',  {.i = index + 1}};
	args[1]  = (struct format_arg){L'u',  L's',  {.s = item->is_unread == true ? "N" : " "}};
	args[2]  = (struct format_arg){L'd',  L's',  {.s = item->date_str->ptr}};
	args[3]  = (struct format_arg){L'D',  L's',  {.s = item->pub_date_str->ptr}};
	args[4]  = (struct format_arg){L'l',  L's',  {.s = item->url->ptr}};
	args[5]  = (struct format_arg){L't',  L's',  {.s = item->title->ptr}};
	args[6]  = (struct format_arg){L'o',  L's',  {.s = item->title->len ? item->title->ptr : item->url->ptr}};
	args[7]  = (struct format_arg){L'L',  L's',  {.s = item->feed[0]->url->ptr}};
	args[8]  = (struct format_arg){L'T',  L's',  {.s = item->feed[0]->name ? item->feed[0]->name->ptr : ""}};
	args[9]  = (struct format_arg){L'O',  L's',  {.s = item->feed[0]->name ? item->feed[0]->name->ptr : item->feed[0]->url->ptr}};
	args[10] = (struct format_arg){L'\0', L'\0', {/* terminator */}};
	return args;
}

static struct config_color
paint_item(struct menu_state *ctx, size_t index, bool is_selected)
{
	struct config_context **cfg = &ctx->items->ptr[index].feed[0]->cfg;
	struct config_color color;
	if (ctx->items->ptr[index].is_important) {
		color = get_cfg_color(cfg, CFG_COLOR_LIST_ITEM_IMPORTANT);
	} else if (ctx->items->ptr[index].is_unread) {
		color = get_cfg_color(cfg, CFG_COLOR_LIST_ITEM_UNREAD);
	} else {
		color = get_cfg_color(cfg, CFG_COLOR_LIST_ITEM);
	}
	if (is_selected) {
		if (is_cfg_color_set(cfg, CFG_COLOR_LIST_ITEM_SELECTED)) {
			color = get_cfg_color(cfg, CFG_COLOR_LIST_ITEM_SELECTED);
		} else {
			color.attributes |= TB_REVERSE;
		}
	}
	return color;
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
mark_all_items_read(struct menu_state *ctx, bool status, int64_t selected_rowid)
{
	pthread_mutex_lock(&interface_lock);
	if (ctx->flags & MENU_IS_SEARCH) {
		obtain_items_at_least_up_to_the_given_index(ctx->items, SIZE_MAX);
		for (size_t i = 0; i < ctx->items->len; ++i) {
			if (db_mark_item_read(ctx->items->ptr[i].rowid, status)) {
				ctx->items->ptr[i].is_unread = !status;
			}
		}
		pthread_mutex_unlock(&interface_lock);
		expose_all_visible_entries_of_the_list_menu();
	} else {
		// Use intermediate variables to avoid race condition
		struct feed_entry **items_feeds = ctx->items->feeds;
		size_t items_feeds_count = ctx->items->feeds_count;
		pthread_mutex_unlock(&interface_lock);
		mark_feeds_read(items_feeds, items_feeds_count, status);
		update_menu_item_list(ctx, selected_rowid);
	}
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
	raise_menu_age();
	if (!m->is_initialized) {
		if (!update_menu_item_list(m, -1)) {
			return close_menu(); // Error displayed by update_menu_item_list()
		}
	}
	start_menu();
	const struct wstring *arg, *browser;
	while (true) {
		if (get_cfg_bool(NULL, CFG_MENU_RESPONSIVENESS) && m->age != fetch_menu_age()) {
			update_menu_item_list(m, m->items->ptr[m->view_sel].rowid);
		}
		if (get_cfg_bool(&m->items->ptr[m->view_sel].feed[0]->cfg, CFG_MARK_ITEM_READ_ON_HOVER)) {
			mark_item_read(m, m->view_sel, true);
		}
		input_id cmd = get_input(m->items->ptr[m->view_sel].feed[0]->binds, NULL, &arg);
		if (handle_list_menu_control(m, cmd, arg)) {
			continue;
		}
		switch (cmd) {
			case INPUT_MARK_READ:         mark_item_read(m, m->view_sel, true);                            break;
			case INPUT_MARK_UNREAD:       mark_item_read(m, m->view_sel, false);                           break;
			case INPUT_TOGGLE_READ:       toggle_item_read(m, m->view_sel);                                break;
			case INPUT_MARK_READ_ALL:     mark_all_items_read(m, true, m->items->ptr[m->view_sel].rowid);  break;
			case INPUT_MARK_UNREAD_ALL:   mark_all_items_read(m, false, m->items->ptr[m->view_sel].rowid); break;
			case INPUT_MARK_IMPORTANT:    mark_item_important(m, m->view_sel, true);                       break;
			case INPUT_MARK_UNIMPORTANT:  mark_item_important(m, m->view_sel, false);                      break;
			case INPUT_TOGGLE_IMPORTANT:  toggle_item_important(m, m->view_sel);                           break;
			case INPUT_RELOAD:            queue_updates(m->items->ptr[m->view_sel].feed, 1);               break;
			case INPUT_RELOAD_ALL:        queue_updates(m->feeds_full, m->feeds_full_size);                break;
			case INPUT_COPY_TO_CLIPBOARD: copy_string_to_clipboard(m->items->ptr[m->view_sel].url);        break;
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
			case INPUT_TOGGLE_READ_FEEDS:
				hide_read_feeds = !hide_read_feeds;
				break;
			case INPUT_TOGGLE_READ_MENU:
			case INPUT_TOGGLE_READ_ITEMS: {
				hide_read_items = !hide_read_items;
				update_menu_item_list(m, m->items->ptr[m->view_sel].rowid);
				break;
			}
			case INPUT_GOTO_FEED:
				if (!(m->flags & MENU_IS_EXPLORE)) break;
				return setup_menu(&items_menu_loop, NULL, m->items->ptr[m->view_sel].feed, 1, MENU_NORMAL, NULL);
			case INPUT_APPLY_SEARCH_MODE_FILTER:
				return setup_menu(&items_menu_loop, NULL, m->feeds_full, m->feeds_full_size, MENU_IS_SEARCH | MENU_IS_EXPLORE, m->find_filter);
			case INPUT_OPEN_IN_BROWSER:
				browser = get_cfg_wstring(&m->items->ptr[m->view_sel].feed[0]->cfg, CFG_OPEN_IN_BROWSER_COMMAND);
				struct format_arg args[100];
				run_formatted_command(browser, get_item_args(m, m->view_sel, args, LENGTH(args)), false);
				break;
			case INPUT_SORT_BY_TIME:
			case INPUT_SORT_BY_TIME_DOWNLOAD:
			case INPUT_SORT_BY_TIME_PUBLICATION:
			case INPUT_SORT_BY_TIME_UPDATE:
			case INPUT_SORT_BY_ROWID:
			case INPUT_SORT_BY_UNREAD:
			case INPUT_SORT_BY_ALPHABET:
			case INPUT_SORT_BY_IMPORTANT:
				change_items_list_sorting(m, cmd, m->items->ptr[m->view_sel].rowid);
				break;
			case INPUT_FIND_COMMAND:
				if (m->find_filter) {
					return setup_menu(&items_menu_loop, NULL, m->feeds_full, m->feeds_full_size, MENU_IS_EXPLORE | MENU_SWALLOW, arg);
				}
				return setup_menu(&items_menu_loop, NULL, m->feeds_full, m->feeds_full_size, MENU_IS_EXPLORE, arg);
			case INPUT_DATABASE_COMMAND:
				db_perform_user_edit(arg, NULL, 0, &m->items->ptr[m->view_sel]);
				break;
			case INPUT_ENTER:
				return setup_menu(&item_pager_loop, m->items->ptr[m->view_sel].title, NULL, 0, MENU_NORMAL, NULL);
		}
	}
	return close_menu();
}
