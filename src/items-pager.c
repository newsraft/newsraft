#include <string.h>
#include "newsraft.h"

static inline bool
populate_render_blocks_list_with_data_from_item(const struct item_entry *item, struct render_blocks_list *blocks, struct links_list *links)
{
	sqlite3_stmt *res = db_find_item_by_rowid(item->rowid);
	if (res == NULL) {
		return false;
	}
	if (populate_link_list_with_links_of_item(links, res) == false) {
		goto error;
	}
	if (generate_render_blocks_based_on_item_data(blocks, item, res) == false) {
		goto error;
	}
	if (prepare_to_render_data(blocks, links) == false) {
		goto error;
	}
	if (links->len > 0) {
		if (complete_urls_of_links(links) == false) {
			goto error;
		}
		struct wstring *links_wstr = generate_link_list_wstring_for_pager(links);
		if (links_wstr == NULL) {
			goto error;
		}
		if (apply_links_render_blocks(blocks, links_wstr) == false) {
			free_wstring(links_wstr);
			goto error;
		}
		free_wstring(links_wstr);
	}
	sqlite3_finalize(res);
	return true;
error:
	sqlite3_finalize(res);
	free_render_blocks(blocks);
	free_links_list(links);
	return false;
}

struct menu_state *
item_pager_loop(struct menu_state *m)
{
	m->enumerator   = &is_pager_pos_valid;
	m->write_action = &pager_menu_writer;
	struct render_blocks_list blocks = {0};
	struct links_list links = {0};
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
	struct item_entry *item = items_menu->items->ptr + item_id;
	struct format_arg items_pager_fmt_args[] = {
		{L'l',  L's',  {.s = NULL}},
		{L'\0', L'\0', {.i = 0   }}, // terminator
	};
	INFO("Trying to view an item with the rowid %" PRId64 "...", item->rowid);
	if (populate_render_blocks_list_with_data_from_item(item, &blocks, &links) == false) {
		goto quit;
	}
	if (start_pager_menu(&blocks) == false) {
		goto quit;
	}
	db_mark_item_read(item->rowid, true);
	item->is_unread = false;
	start_menu();
	uint32_t count;
	const struct wstring *macro;
	for (input_cmd_id cmd = get_input_cmd(&count, &macro) ;; cmd = get_input_cmd(&count, &macro)) {
		if (handle_pager_menu_control(cmd) == true) {
			// Rest a little.
		} else if ((cmd == INPUT_JUMP_TO_NEXT)
			|| (cmd == INPUT_JUMP_TO_PREV)
			|| (cmd == INPUT_JUMP_TO_NEXT_UNREAD)
			|| (cmd == INPUT_JUMP_TO_PREV_UNREAD)
			|| (cmd == INPUT_JUMP_TO_NEXT_IMPORTANT)
			|| (cmd == INPUT_JUMP_TO_PREV_IMPORTANT))
		{
			handle_list_menu_control(items_menu, cmd, NULL);
			if (items_menu->view_sel != item_id) {
				free_render_blocks(&blocks);
				free_links_list(&links);
				return setup_menu(&item_pager_loop, NULL, 0, MENU_SWALLOW);
			}
		} else if (cmd == INPUT_NAVIGATE_BACK || cmd == INPUT_QUIT_SOFT || cmd == INPUT_QUIT_HARD) {
			free_render_blocks(&blocks);
			free_links_list(&links);
			return cmd == INPUT_QUIT_HARD ? NULL : setup_menu(NULL, NULL, 0, 0);
		} else if (cmd == INPUT_OPEN_IN_BROWSER && count > 0 && count <= links.len) {
			items_pager_fmt_args[0].value.s = links.ptr[count - 1].url->ptr;
			run_formatted_command(get_cfg_wstring(NULL, CFG_OPEN_IN_BROWSER_COMMAND), items_pager_fmt_args);
		} else if (cmd == INPUT_COPY_TO_CLIPBOARD && count > 0 && count <= links.len) {
			copy_string_to_clipboard(links.ptr[count - 1].url);
		} else if (cmd == INPUT_SYSTEM_COMMAND && count > 0 && count <= links.len) {
			items_pager_fmt_args[0].value.s = links.ptr[count - 1].url->ptr;
			run_formatted_command(macro, items_pager_fmt_args);
		}
	}
quit:
	free_render_blocks(&blocks);
	free_links_list(&links);
	return setup_menu(NULL, NULL, 0, 0);
}
