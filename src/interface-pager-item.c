#include <stdlib.h>
#include "newsraft.h"

static struct format_arg cmd_args[] = {
	{L'l',  L"s", {.s = NULL}},
	{L'\0', NULL, {.i = 0}}, // terminator
};

static inline bool
join_links_render_block(struct render_block **contents, struct link_list *links)
{
	struct string *str = generate_link_list_string_for_pager(links);
	if (str == NULL) {
		return false;
	}
	join_render_separator(contents);
	join_render_separator(contents);
	join_render_block(contents, str->ptr, str->len, TEXT_PLAIN);
	join_render_separator(contents);
	free_string(str);
	return true;
}

static inline struct render_block *
generate_render_blocks_for_item(sqlite3_stmt *res, struct link_list *links)
{
	struct render_block *first_block = NULL;
	if (populate_link_list_with_links_of_item(links, res) == false) {
		goto error;
	}
	if (join_render_blocks_of_item_data(&first_block, res) == false) {
		goto error;
	}
	if (prepare_to_render_data(first_block, links) == false) {
		goto error;
	}
	if (links->len != 0) {
		if (complete_urls_of_links(links, res) == false) {
			goto error;
		}
		if (join_links_render_block(&first_block, links) == false) {
			goto error;
		}
	}
	return first_block;
error:
	free_render_blocks(first_block);
	free_trim_link_list(links);
	return NULL;
}

static bool
custom_input_handler(void *data, input_cmd_id cmd, uint32_t count, const struct wstring *macro)
{
	const struct link_list *links = data;
	if ((count != 0) && (links->len >= count)) {
		if (cmd == INPUT_OPEN_IN_BROWSER) {
			return open_url_in_browser(links->list[count - 1].url);
		} else if (cmd == INPUT_COPY_TO_CLIPBOARD) {
			return copy_string_to_clipboard(links->list[count - 1].url);
		} else if (cmd == INPUT_SYSTEM_COMMAND) {
			cmd_args[0].value.s = links->list[count - 1].url->ptr;
			return execute_command_with_specifiers_in_it(macro, cmd_args);
		}
	}
	return false;
}

int
enter_item_pager_view_loop(int64_t rowid)
{
	INFO("Trying to view an item with rowid %" PRId64 "...", rowid);
	sqlite3_stmt *res = db_find_item_by_rowid(rowid);
	if (res == NULL) {
		return INPUT_ERROR;
	}
	struct link_list links = {0};
	struct render_block *block = generate_render_blocks_for_item(res, &links);
	sqlite3_finalize(res);
	if (block == NULL) {
		return INPUT_ERROR;
	}
	const int pager_result = pager_view(block, &custom_input_handler, (void *)&links);
	free_render_blocks(block);
	free_trim_link_list(&links);
	return pager_result;
}
