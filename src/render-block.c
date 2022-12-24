#include <stdlib.h>
#include "newsraft.h"

bool
join_render_block(struct render_blocks_list *blocks, const char *content, size_t content_len, int8_t content_type)
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
	blocks->ptr[blocks->len].content_type = content_type;
	blocks->len += 1;
	return true;
}

bool
join_render_separator(struct render_blocks_list *blocks)
{
	return join_render_block(blocks, "", 0, TEXT_SEPARATOR);
}

void
free_render_blocks(struct render_blocks_list *blocks)
{
	for (size_t i = 0; i < blocks->len; ++i) {
		free_wstring(blocks->ptr[i].content);
	}
	free(blocks->ptr);
	free(blocks->hints);
}
