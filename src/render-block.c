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
join_render_block(struct render_block **list, const char *content, size_t content_len, int8_t content_type)
{
	struct render_block *new_entry = malloc(sizeof(struct render_block));
	if (new_entry == NULL) {
		return false;
	}
	new_entry->content = convert_array_to_wstring(content, content_len);
	if (new_entry->content == NULL) {
		free(new_entry);
		return false;
	}
	new_entry->content_type = content_type;
	new_entry->next = NULL;
	struct render_block **last_block = list;
	while (*last_block != NULL) {
		last_block = &((*last_block)->next);
	}
	*last_block = new_entry;
	return true;
}

bool
join_render_separator(struct render_block **list)
{
	return join_render_block(list, "", 0, TEXT_SEPARATOR);
}

void
free_render_blocks(struct render_block *first_block)
{
	struct render_block *head_block = first_block;
	struct render_block *temp;
	while (head_block != NULL) {
		free_wstring(head_block->content);
		temp = head_block;
		head_block = head_block->next;
		free(temp);
	}
}
