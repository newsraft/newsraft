#include <stdlib.h>
#include "newsraft.h"

bool
add_render_block(struct render_blocks_list *blocks, const char *content, size_t content_len, render_block_format content_type, bool needs_trimming)
{
	if (content == NULL || content_len == 0) {
		return true; // Ignore empty render blocks.
	}
	void *tmp = realloc(blocks->ptr, sizeof(struct render_block) * (blocks->len + 1));
	if (tmp == NULL) {
		return false;
	}
	blocks->ptr = tmp;
	blocks->ptr[blocks->len].content = convert_array_to_wstring(content, content_len);
	if (blocks->ptr[blocks->len].content == NULL) {
		return false;
	}
	blocks->ptr[blocks->len].content_type = content_type;
	blocks->ptr[blocks->len].needs_trimming = needs_trimming;
	blocks->len += 1;
	return true;
}

void
apply_links_render_blocks(struct render_blocks_list *blocks, const struct wstring *data)
{
	if (data != NULL && data->ptr != NULL && data->len > 0) {
		for (size_t i = 0; i < blocks->len; ++i) {
			if (blocks->ptr[i].content_type == TEXT_LINKS) {
				wstr_set(&blocks->ptr[i].content, data->ptr, data->len, data->len);
				blocks->ptr[i].content_type = TEXT_HTML;
			}
		}
	}
}

void
free_render_blocks(struct render_blocks_list *blocks)
{
	for (size_t i = 0; i < blocks->len; ++i) {
		free_wstring(blocks->ptr[i].content);
	}
	free(blocks->ptr);
	if (blocks->links.ptr != NULL) {
		for (size_t i = 0; i < blocks->links.len; ++i) {
			free_contents_of_link(&blocks->links.ptr[i]);
		}
		free(blocks->links.ptr);
		blocks->links.ptr = NULL;
		blocks->links.len = 0;
	}
}

void
free_render_result(struct render_result *render)
{
	if (render != NULL) {
		for (size_t i = 0; i < render->lines_len; ++i) {
			free_wstring(render->lines[i].ws);
			free(render->lines[i].hints);
		}
		free(render->lines);
		render->lines = NULL;
		render->lines_len = 0;
	}
}
