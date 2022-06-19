#include "newsraft.h"

static inline bool
join_links_render_block(struct render_block **contents, struct link_list *links)
{
	struct string *str = generate_link_list_string_for_pager(links);
	if (str == NULL) {
		return false;
	}
	join_render_separator(contents);
	join_render_separator(contents);
	join_render_block(contents, str->ptr, str->len, "text/plain", 10);
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

static inline const char *
number_suffix(uint32_t n)
{
	if ((n > 3) && (n < 21)) {
		return "th";
	}
	uint32_t mod = n % 10;
	if (mod == 1) {
		return "st";
	} else if (mod == 2) {
		return "nd";
	} else if (mod == 3) {
		return "rd";
	} else {
		return "th";
	}
}

static bool
custom_input_handler(void *data, input_cmd_id cmd, uint32_t count)
{
	const struct link_list *links = data;
	if ((count != 0) && (links->len >= count) && (links->list[count - 1].url != NULL)) {
		if (cmd == INPUT_OPEN_IN_BROWSER) {
			open_url_in_browser(links->list[count - 1].url);
			return true;
		} else if (cmd == INPUT_COPY_TO_CLIPBOARD) {
			if (copy_string_to_clipboard(links->list[count - 1].url) == true) {
				good_status("Copied %" PRIu32 "%s link to clipboard.", count, number_suffix(count));
			}
			return false;
		}
	}
	return false;
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
	reverse_render_blocks(&block);
	const int pager_result = pager_view(block, &custom_input_handler, (void *)&links);
	free_render_blocks(block);
	free_trim_link_list(&links);
	return pager_result;
}
