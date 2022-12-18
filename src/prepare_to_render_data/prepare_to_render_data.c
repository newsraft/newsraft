#include "prepare_to_render_data/prepare_to_render_data.h"

bool
prepare_to_render_data(struct render_blocks_list *blocks, struct links_list *links)
{
	INFO("Preparing to render data...");
	struct wstring *processed_str;
	for (size_t i = 0; i < blocks->len; ++i) {
		if (blocks->ptr[i].content_type == TEXT_HTML) {
			processed_str = prepare_to_render_text_html(blocks->ptr[i].content, links);
			if (processed_str == NULL) {
				FAIL("Failed to prepare HTML text for rendering!");
				return false;
			}
			free_wstring(blocks->ptr[i].content);
			blocks->ptr[i].content = processed_str;
		}
	}
	return true;
}
