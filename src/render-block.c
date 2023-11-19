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

bool
apply_links_render_blocks(struct render_blocks_list *blocks, const struct string *data)
{
	if (blocks->len == 0 || blocks->links_block_index == 0 || data == NULL || data->ptr == NULL || data->len == 0) {
		return true; // Ignore when links block is not needed or when it is empty.
	}
	void *tmp = realloc(blocks->ptr, sizeof(struct render_block) * (blocks->len + 3));
	if (tmp == NULL) {
		return false;
	}
	blocks->ptr = tmp;
	for (size_t i = blocks->len - 1; i >= blocks->links_block_index; --i) {
		blocks->ptr[i + 3].content        = blocks->ptr[i].content;
		blocks->ptr[i + 3].content_type   = blocks->ptr[i].content_type;
		blocks->ptr[i + 3].needs_trimming = blocks->ptr[i].needs_trimming;
	}
	blocks->ptr[blocks->links_block_index + 0].content        = convert_string_to_wstring(blocks->pre_links_block);
	blocks->ptr[blocks->links_block_index + 0].content_type   = TEXT_HTML;
	blocks->ptr[blocks->links_block_index + 0].needs_trimming = false;
	blocks->ptr[blocks->links_block_index + 1].content        = convert_string_to_wstring(data);
	blocks->ptr[blocks->links_block_index + 1].content_type   = TEXT_RAW;
	blocks->ptr[blocks->links_block_index + 1].needs_trimming = true;
	blocks->ptr[blocks->links_block_index + 2].content        = convert_string_to_wstring(blocks->post_links_block);
	blocks->ptr[blocks->links_block_index + 2].content_type   = TEXT_HTML;
	blocks->ptr[blocks->links_block_index + 2].needs_trimming = false;
	blocks->len += 3;
	return true;
}

void
free_render_blocks(struct render_blocks_list *blocks)
{
	for (size_t i = 0; i < blocks->len; ++i) {
		free_wstring(blocks->ptr[i].content);
	}
	free(blocks->ptr);
	free_string(blocks->pre_links_block);
	free_string(blocks->post_links_block);
}
