#include "prepare_to_render_data/prepare_to_render_data.h"

bool
prepare_to_render_data(struct render_block *first_block, struct link_list *links)
{
	INFO("Preparing to render data.");
	struct wstring *processed_str;
	struct render_block *block = first_block;
	while (block != NULL) {
		if (block->content_type == TEXT_HTML) {
			processed_str = prepare_to_render_text_html(block->content, links);
			if (processed_str == NULL) {
				return false;
			}
			free_wstring(block->content);
			block->content = processed_str;
		}
		block = block->next;
	}
	INFO("Finished preparing to render data.");
	return true;
}
