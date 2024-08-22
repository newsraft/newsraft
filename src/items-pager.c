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
	m->enumerator   = &is_pager_pos_valid;
	m->write_action = &pager_menu_writer;
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
	struct format_arg items_pager_fmt_args[] = {
		{L'l',  L's',  {.s = NULL}},
		{L'\0', L'\0', {.i = 0   }}, // terminator
	};
	INFO("Trying to view an item with the rowid %" PRId64 "...", items_menu->items->ptr[item_id].rowid);
	if (populate_render_blocks_list_with_data_from_item(&items_menu->items->ptr[item_id], &blocks) == false) {
		goto quit;
	}
	if (start_pager_menu(&items_menu->items->ptr[item_id].feed[0]->cfg, &blocks) == false) {
		goto quit;
	}
	db_mark_item_read(items_menu->items->ptr[item_id].rowid, true);
	items_menu->items->ptr[item_id].is_unread = false;
	start_menu();
	uint32_t count;
	const struct wstring *macro;
	while (true) {
		input_id cmd = get_input(items_menu->items->ptr[item_id].feed[0]->binds, &count, &macro);
		if (handle_pager_menu_control(cmd) == true) {
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
					return setup_menu(&item_pager_loop, items_menu->items->ptr[items_menu->view_sel].title, NULL, 0, MENU_SWALLOW);
				}
				break;
			case INPUT_NAVIGATE_BACK:
			case INPUT_QUIT_SOFT:
			case INPUT_QUIT_HARD:
				free_render_blocks(&blocks);
				return cmd == INPUT_QUIT_HARD ? NULL : close_menu();
			case INPUT_OPEN_IN_BROWSER:
				if (count > 0 && count <= blocks.links.len) {
					struct config_context **cfg = &items_menu->items->ptr[item_id].feed[0]->cfg;
					const struct wstring *browser = get_cfg_wstring(cfg, CFG_OPEN_IN_BROWSER_COMMAND);
					items_pager_fmt_args[0].value.s = blocks.links.ptr[count - 1].url->ptr;
					run_formatted_command(browser, items_pager_fmt_args);
				}
				break;
			case INPUT_COPY_TO_CLIPBOARD:
				if (count > 0 && count <= blocks.links.len) {
					copy_string_to_clipboard(blocks.links.ptr[count - 1].url);
				}
				break;
			case INPUT_SYSTEM_COMMAND:
				if (count > 0 && count <= blocks.links.len) {
					items_pager_fmt_args[0].value.s = blocks.links.ptr[count - 1].url->ptr;
					run_formatted_command(macro, items_pager_fmt_args);
				}
				break;
			default:
				break;
		}
	}
quit:
	free_render_blocks(&blocks);
	return close_menu();
}
