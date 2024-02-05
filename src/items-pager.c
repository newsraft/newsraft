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

int
enter_item_pager_view_loop(struct items_list *items, const size_t *view_sel)
{
	struct format_arg items_pager_fmt_args[] = {
		{L'l',  L's',  {.s = NULL}},
		{L'\0', L'\0', {.i = 0   }}, // terminator
	};
	struct render_blocks_list blocks;
	struct links_list links;
	while (true) {
		INFO("Trying to view an item with the rowid %" PRId64 "...", items->ptr[*view_sel].rowid);
		memset(&blocks, 0, sizeof(struct render_blocks_list));
		memset(&links, 0, sizeof(struct links_list));
		if (populate_render_blocks_list_with_data_from_item(items->ptr + *view_sel, &blocks, &links) == false) {
			return INPUT_ERROR;
		}
		if (start_pager_menu(&blocks) == false) {
			free_render_blocks(&blocks);
			free_links_list(&links);
			return INPUT_ERROR;
		}
		db_mark_item_read(items->ptr[*view_sel].rowid, true);
		items->ptr[*view_sel].is_unread = false;
		enter_list_menu(PAGER_MENU, CFG_MENU_SECTION_ENTRY_FORMAT, true);
		while (true) {
			uint32_t count;
			const struct wstring *macro;
			input_cmd_id cmd = get_input_cmd(&count, &macro);
			if (handle_pager_menu_control(cmd) == true) {
				// Rest a little.
			} else if ((cmd == INPUT_JUMP_TO_NEXT)
				|| (cmd == INPUT_JUMP_TO_PREV)
				|| (cmd == INPUT_JUMP_TO_NEXT_UNREAD)
				|| (cmd == INPUT_JUMP_TO_PREV_UNREAD)
				|| (cmd == INPUT_JUMP_TO_NEXT_IMPORTANT)
				|| (cmd == INPUT_JUMP_TO_PREV_IMPORTANT))
			{
				size_t current_sel = *view_sel;
				handle_list_menu_control(ITEMS_MENU, cmd, NULL);
				if (current_sel != *view_sel) {
					break;
				}
			} else if (cmd == INPUT_NAVIGATE_BACK || cmd == INPUT_QUIT_SOFT || cmd == INPUT_QUIT_HARD) {
				free_render_blocks(&blocks);
				free_links_list(&links);
				return cmd;
			} else if (cmd == INPUT_OPEN_IN_BROWSER && count > 0 && count <= links.len) {
				items_pager_fmt_args[0].value.s = links.ptr[count - 1].url->ptr;
				run_formatted_command(get_cfg_wstring(CFG_OPEN_IN_BROWSER_COMMAND), items_pager_fmt_args);
			} else if (cmd == INPUT_COPY_TO_CLIPBOARD && count > 0 && count <= links.len) {
				copy_string_to_clipboard(links.ptr[count - 1].url);
			} else if (cmd == INPUT_SYSTEM_COMMAND && count > 0 && count <= links.len) {
				items_pager_fmt_args[0].value.s = links.ptr[count - 1].url->ptr;
				run_formatted_command(macro, items_pager_fmt_args);
			}
		}
		free_render_blocks(&blocks);
		free_links_list(&links);
	}
	return INPUT_ERROR; // It shouldn't reach here.
}
