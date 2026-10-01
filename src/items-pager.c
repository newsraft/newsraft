#include <string.h>
#include "newsraft.h"

static inline bool
populate_render_blocks_list_with_data_from_item(const struct item_entry *item, struct render_blocks_list *blocks)
{
	sqlite3_stmt *res = db_find_item_by_rowid(item->rowid);
	if (res == NULL) {
		return false;
	}
	if (item->url != NULL && item->url->len > 0) {
		if (add_url_to_links_list(&blocks->links, item->url->ptr, item->url->len) < 0) {
			goto error;
		}
	}
	if (add_item_attachments_to_links_list(&blocks->links, res) == false) {
		goto error;
	}
	if (generate_render_blocks_based_on_item_data(blocks, item, res) == false) {
		goto error;
	}
	start_pager_menu(&item->feed[0]->cfg, blocks);
	if (blocks->links.len > 0) {
		struct wstring *links_wstr = generate_link_list_wstring_for_pager(&item->feed[0]->cfg, &blocks->links);
		if (links_wstr == NULL) {
			goto error;
		}
		apply_links_render_blocks(blocks, links_wstr);
		free_wstring(links_wstr);
	}
	sqlite3_finalize(res);
	return true;
error:
	sqlite3_finalize(res);
	free_render_blocks(blocks);
	return false;
}

struct menu_state *
item_pager_loop(struct menu_state *m)
{
	m->enumerator = &is_pager_pos_valid;
	m->printer    = &pager_menu_writer;
	struct render_blocks_list blocks = {0};
	struct menu_state *items_menu = NULL;
	size_t item_id = 0;
	for (struct menu_state *i = m->prev; i != NULL; i = i->prev) {
		if (i->items != NULL) {
			items_menu = i;
			item_id = i->view_sel;
			break;
		}
	}
	if (items_menu == NULL) {
		goto quit;
	}
	INFO("Trying to view an item with the rowid %" PRId64 "...", items_menu->items->ptr[item_id].rowid);
	if (populate_render_blocks_list_with_data_from_item(&items_menu->items->ptr[item_id], &blocks) == false) {
		goto quit;
	}
	if (start_pager_menu(&items_menu->items->ptr[item_id].feed[0]->cfg, &blocks) == false) {
		goto quit;
	}
	db_mark_item_read(items_menu->items->ptr[item_id].rowid, true);
	items_menu->items->ptr[item_id].is_unread = false;
	menu_start(m);
	input_id cmd;
	uint32_t count;
	const struct wstring *arg;
	while (menu_read(items_menu->items->ptr[item_id].feed[0]->binds, &cmd, &count, &arg)) {
		if (handle_pager_menu_control(cmd)) {
			continue;
		}
		switch (cmd) {
			case INPUT_JUMP_TO_NEXT:
			case INPUT_JUMP_TO_PREV:
			case INPUT_JUMP_TO_NEXT_UNREAD:
			case INPUT_JUMP_TO_PREV_UNREAD:
			case INPUT_JUMP_TO_NEXT_IMPORTANT:
			case INPUT_JUMP_TO_PREV_IMPORTANT:
				handle_list_menu_control(items_menu, cmd, NULL);
				if (items_menu->view_sel != item_id) {
					free_render_blocks(&blocks);
					return menu_setup(&item_pager_loop, items_menu->items->ptr[items_menu->view_sel].title, NULL, 0, MENU_SWALLOW, NULL);
				}
				break;
			case INPUT_NAVIGATE_BACK:
			case INPUT_QUIT_SOFT:
			case INPUT_QUIT_HARD:
				free_render_blocks(&blocks);
				return cmd == INPUT_QUIT_HARD ? NULL : menu_close();
			case INPUT_COPY_TO_CLIPBOARD:
				if (count > 0 && count <= blocks.links.len) {
					copy_string_to_clipboard(blocks.links.ptr[count - 1].url);
				}
				break;
			case INPUT_OPEN_IN_BROWSER:
			case INPUT_SYSTEM_COMMAND:
			case INPUT_SYSTEM_COMMAND_QUIET: {
				if (count <= 0 || count > blocks.links.len) {
					break;
				}
				struct format_arg args[100];
				const char *url = blocks.links.ptr[count - 1].url->ptr;
				struct config_context **cfg = &items_menu->items->ptr[item_id].feed[0]->cfg;
				const struct wstring *browser = get_cfg_wstring(cfg, CFG_OPEN_IN_BROWSER_COMMAND);
				items_menu->get_args(items_menu, item_id, args, LENGTH(args));
				for (size_t i = 0; i < LENGTH(args); ++i) {
					if (args[i].specifier == L'l') {
						args[i].value.s = url;
						break;
					}
				}
				run_formatted_command(
					cmd == INPUT_OPEN_IN_BROWSER ? browser : arg,
					args,
					cmd == INPUT_SYSTEM_COMMAND_QUIET
				);
				break;
			}
			case INPUT_MARK_READ:
				if (items_menu->items->ptr[item_id].is_unread) {
					if (db_mark_item_read(items_menu->items->ptr[item_id].rowid, true)) {
						items_menu->items->ptr[item_id].is_unread = false;
					}
				}
				break;
			case INPUT_MARK_UNREAD:
				if (!items_menu->items->ptr[item_id].is_unread) {
					if (db_mark_item_read(items_menu->items->ptr[item_id].rowid, false)) {
						items_menu->items->ptr[item_id].is_unread = true;
					}
				}
				break;
			case INPUT_TOGGLE_READ: {
				bool was_unread = items_menu->items->ptr[item_id].is_unread;
				if (db_mark_item_read(items_menu->items->ptr[item_id].rowid, was_unread)) {
					items_menu->items->ptr[item_id].is_unread = !was_unread;

					struct config_context **cfg = &items_menu->items->ptr[item_id].feed[0]->cfg;
					if (get_cfg_bool(cfg, CFG_CLOSE_PAGER_ON_TOGGLE_READ)) {
						goto quit;
					}
				}
				break;
			}
			case INPUT_MARK_IMPORTANT:
				if (!items_menu->items->ptr[item_id].is_important) {
					if (db_mark_item_important(items_menu->items->ptr[item_id].rowid, true)) {
						items_menu->items->ptr[item_id].is_important = true;
					}
				}
				break;
			case INPUT_MARK_UNIMPORTANT:
				if (items_menu->items->ptr[item_id].is_important) {
					if (db_mark_item_important(items_menu->items->ptr[item_id].rowid, false)) {
						items_menu->items->ptr[item_id].is_important = false;
					}
				}
				break;
			case INPUT_TOGGLE_IMPORTANT: {
				bool new_important = !items_menu->items->ptr[item_id].is_important;
				if (db_mark_item_important(items_menu->items->ptr[item_id].rowid, new_important)) {
					items_menu->items->ptr[item_id].is_important = new_important;

					struct config_context **cfg = &items_menu->items->ptr[item_id].feed[0]->cfg;
					if (get_cfg_bool(cfg, CFG_CLOSE_PAGER_ON_TOGGLE_IMPORTANT)) {
						goto quit;
					}
				}
				break;
			}
			default:
				break;
		}
	}
quit:
	free_render_blocks(&blocks);
	return menu_close();
}
