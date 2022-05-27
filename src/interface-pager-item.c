#include "newsraft.h"

static inline struct render_block *
generate_render_blocks_for_item(sqlite3_stmt *res)
{
	struct render_block *first_block = NULL;
	struct link_list links = {NULL, 0, 0};
	if (populate_link_list_with_links_of_item(&links, res) == false) {
		goto error;
	}
	if (join_render_blocks_of_item_data(&first_block, res) == false) {
		goto error;
	}
	if (prepare_to_render_data(first_block, &links) == false) {
		goto error;
	}
	if (get_cfg_bool(CFG_CONTENT_APPEND_LINKS) == true) {
		if (complete_urls_of_links(&links, res) == false) {
			goto error;
		}
		if (join_links_render_block(&first_block, &links) == false) {
			goto error;
		}
	}
	free_trim_link_list(&links);
	return first_block;
error:
	free_render_blocks(first_block);
	free_trim_link_list(&links);
	return NULL;
}

int
enter_item_pager_view_loop(int rowid)
{
	INFO("Trying to view an item with rowid %d...", rowid);
	sqlite3_stmt *res = db_find_item_by_rowid(rowid);
	if (res == NULL) {
		return INPUTS_COUNT;
	}
	struct render_block *block = generate_render_blocks_for_item(res);
	sqlite3_finalize(res);
	if (block == NULL) {
		return INPUTS_COUNT;
	}
	const int pager_result = pager_view(block);
	free_render_blocks(block);
	return pager_result;
}
