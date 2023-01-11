#include <string.h>
#include "newsraft.h"

static struct format_arg cmd_args[] = {
	{L'l',  L"s", {.s = NULL}},
	{L'\0', NULL, {.i = 0}}, // terminator
};

static inline bool
join_links_render_block(struct render_blocks_list *blocks, struct links_list *links)
{
	struct string *str = generate_link_list_string_for_pager(links);
	if (str == NULL) {
		return false;
	}
	join_render_separator(blocks);
	join_render_block(blocks, str->ptr, str->len, TEXT_RAW);
	free_string(str);
	return true;
}

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
	if (links->len != 0) {
		if (complete_urls_of_links(links, item->feed->link) == false) {
			goto error;
		}
		if (join_links_render_block(blocks, links) == false) {
			goto error;
		}
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
enter_item_pager_view_loop(struct item_entry *items, const size_t *view_sel)
{
	struct render_blocks_list blocks;
	struct links_list links;
	input_cmd_id cmd;
	uint32_t count;
	const struct wstring *macro;
	size_t current_sel;
	while (true) {
		INFO("Trying to view an item with the rowid %" PRId64 "...", items[*view_sel].rowid);
		memset(&blocks, 0, sizeof(struct render_blocks_list));
		memset(&links, 0, sizeof(struct links_list));
		if (populate_render_blocks_list_with_data_from_item(items + *view_sel, &blocks, &links) == false) {
			return INPUT_ERROR;
		}
		if (start_pager_menu(&blocks) == false) {
			free_render_blocks(&blocks);
			free_links_list(&links);
			return INPUT_ERROR;
		}
		db_mark_item_read(items[*view_sel].rowid);
		items[*view_sel].is_unread = false;
		while (true) {
			cmd = get_input_command(&count, &macro);
			if (handle_pager_menu_navigation(cmd) == true) {
				// Rest a little.
			} else if ((cmd == INPUT_JUMP_TO_NEXT)
				|| (cmd == INPUT_JUMP_TO_PREV)
				|| (cmd == INPUT_JUMP_TO_NEXT_UNREAD)
				|| (cmd == INPUT_JUMP_TO_PREV_UNREAD)
				|| (cmd == INPUT_JUMP_TO_NEXT_IMPORTANT)
				|| (cmd == INPUT_JUMP_TO_PREV_IMPORTANT))
			{
				current_sel = *view_sel;
				handle_list_menu_navigation(cmd);
				if (current_sel != *view_sel) {
					break;
				}
			} else if ((cmd == INPUT_QUIT_SOFT) || (cmd == INPUT_QUIT_HARD)) {
				free_render_blocks(&blocks);
				free_links_list(&links);
				return cmd;
			} else if ((count > 0) && (count <= links.len)) {
				cmd_args[0].value.s = links.ptr[count - 1].url->ptr;
				if (cmd == INPUT_OPEN_IN_BROWSER) {
					run_command_with_specifiers(get_cfg_wstring(CFG_OPEN_IN_BROWSER_COMMAND), cmd_args);
					handle_pager_menu_navigation(INPUT_RESIZE);
				} else if (cmd == INPUT_COPY_TO_CLIPBOARD) {
					copy_string_to_clipboard(links.ptr[count - 1].url);
				} else if (cmd == INPUT_SYSTEM_COMMAND) {
					run_command_with_specifiers(macro, cmd_args);
					handle_pager_menu_navigation(INPUT_RESIZE);
				}
			}
		}
		free_render_blocks(&blocks);
		free_links_list(&links);
	}
	return INPUT_ERROR; // It shouldn't reach here.
}
