#include <stdlib.h>
#include "newsraft.h"

bool
join_render_block(struct render_blocks_list *blocks, const char *content, size_t content_len, render_block_format content_type, size_t separators_count)
{
	void *tmp = realloc(blocks->ptr, sizeof(struct render_block) * (blocks->len + 1));
	if (tmp == NULL) {
		return false;
	}
	blocks->ptr = tmp;
	blocks->ptr[blocks->len].content = convert_array_to_wstring(content, content_len);
	if (blocks->ptr[blocks->len].content == NULL) {
		return false;
	}
	if (blocks->len > 0) {
		blocks->ptr[blocks->len - 1].separators_count = separators_count;
	}
	blocks->ptr[blocks->len].content_type = content_type;
	blocks->ptr[blocks->len].separators_count = 0;
	blocks->len += 1;
	return true;
}

void
free_render_blocks(struct render_blocks_list *blocks)
{
	for (size_t i = 0; i < blocks->len; ++i) {
		free_wstring(blocks->ptr[i].content);
	}
	free(blocks->ptr);
}
