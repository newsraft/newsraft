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
	join_render_separator(blocks);
	join_render_block(blocks, str->ptr, str->len, TEXT_PLAIN);
	join_render_separator(blocks);
	free_string(str);
	return true;
}

static inline bool
populate_render_blocks_list_with_data_from_item(int64_t rowid, struct render_blocks_list *blocks, struct links_list *links)
{
	sqlite3_stmt *res = db_find_item_by_rowid(rowid);
	if (res == NULL) {
		return false;
	}
	if (populate_link_list_with_links_of_item(links, res) == false) {
		goto error;
	}
	if (join_render_blocks_of_item_data(blocks, res) == false) {
		goto error;
	}
	if (prepare_to_render_data(blocks, links) == false) {
		goto error;
	}
	if (links->len != 0) {
		if (complete_urls_of_links(links, res) == false) {
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
	free_trim_link_list(links);
	return false;
}

static bool
custom_input_handler(void *data, input_cmd_id cmd, uint32_t count, const struct wstring *macro)
{
	const struct links_list *links = data;
	if ((count > 0) && (count <= links->len)) {
		if (cmd == INPUT_OPEN_IN_BROWSER) {
			return open_url_in_browser(links->ptr[count - 1].url);
		} else if (cmd == INPUT_COPY_TO_CLIPBOARD) {
			return copy_string_to_clipboard(links->ptr[count - 1].url);
		} else if (cmd == INPUT_SYSTEM_COMMAND) {
			cmd_args[0].value.s = links->ptr[count - 1].url->ptr;
			return execute_command_with_specifiers_in_it(macro, cmd_args);
		}
	}
	return false;
}

int
enter_item_pager_view_loop(int64_t rowid)
{
	INFO("Trying to view an item with the rowid %" PRId64 "...", rowid);
	struct render_blocks_list blocks = {0};
	struct links_list links = {0};
	if (populate_render_blocks_list_with_data_from_item(rowid, &blocks, &links) == false) {
		return INPUT_ERROR;
	}
	const int pager_result = pager_view(&blocks, &custom_input_handler, (void *)&links);
	free_render_blocks(&blocks);
	free_trim_link_list(&links);
	return pager_result;
}
