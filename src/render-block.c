#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

struct content_type_match {
	const char *const name;
	int8_t type;
};

static const struct content_type_match types[] = {
	{"html",       TEXT_HTML},
	{"text/html",  TEXT_HTML},
	{"plain",      TEXT_PLAIN},
	{"text/plain", TEXT_PLAIN},
	{NULL,         0},
};

int8_t
get_content_type_by_string(const char *type)
{
	for (size_t i = 0; types[i].name != NULL; ++i) {
		if (strcmp(type, types[i].name) == 0) {
			return types[i].type;
		}
	}
	return TEXT_PLAIN;
}

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
