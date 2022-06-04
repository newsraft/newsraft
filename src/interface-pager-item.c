#include <stdio.h>
#include "newsraft.h"

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
	if (get_cfg_bool(CFG_CONTENT_APPEND_LINKS) == true) {
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

static inline void
copy_string_to_clipboard(const struct string *src)
{
	const struct string *copy_cmd = get_cfg_string(CFG_COPY_TO_CLIPBOARD_COMMAND);
	FILE *p = popen(copy_cmd->ptr, "w");
	if (p == NULL) {
		return;
	}
	fwrite(src->ptr, sizeof(char), src->len, p);
	pclose(p);
}

static void
custom_input_handler(void *data, input_cmd_id cmd)
{
	if (cmd == INPUT_COPY_TO_CLIPBOARD) {
		const struct link_list *links = data;
		if ((links->len > 0) && (links->list[0].url != NULL)) {
			copy_string_to_clipboard(links->list[0].url);
		}
	}
}

int
enter_item_pager_view_loop(int rowid)
{
	INFO("Trying to view an item with rowid %d...", rowid);
	sqlite3_stmt *res = db_find_item_by_rowid(rowid);
	if (res == NULL) {
		return INPUTS_COUNT;
	}
	struct link_list links = {0};
	struct render_block *block = generate_render_blocks_for_item(res, &links);
	sqlite3_finalize(res);
	if (block == NULL) {
		return INPUTS_COUNT;
	}
	const int pager_result = pager_view(block, &custom_input_handler, (void *)&links);
	free_render_blocks(block);
	free_trim_link_list(&links);
	return pager_result;
}
