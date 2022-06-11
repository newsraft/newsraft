#include <stdlib.h>
#include "newsraft.h"

bool
join_render_block(struct render_block **list, const char *content, size_t content_len, const char *content_type, size_t content_type_len)
{
	struct render_block *new_entry = malloc(sizeof(struct render_block));
	if (new_entry == NULL) {
		return false;
	}
	struct string *str = crtas(content, content_len);
	if (str == NULL) {
		free(new_entry);
		return false;
	}
	new_entry->content = convert_string_to_wstring(str);
	free_string(str);
	if (new_entry->content == NULL) {
		free(new_entry);
		return false;
	}
	new_entry->content_type = malloc(sizeof(char) * (content_type_len + 1));
	if (new_entry->content_type == NULL) {
		free_wstring(new_entry->content);
		free(new_entry);
		return false;
	}
	memcpy(new_entry->content_type, content_type, sizeof(char) * content_type_len);
	new_entry->content_type[content_type_len] = '\0';
	new_entry->next = *list;
	*list = new_entry;
	return true;
}

void
reverse_render_blocks(struct render_block **list)
{
	struct render_block *prev = NULL;
	struct render_block *current = *list;
	struct render_block *next = NULL;
	while (current != NULL) {
		next = current->next;
		current->next = prev;
		prev = current;
		current = next;
	}
	*list = prev;
}

bool
join_render_separator(struct render_block **list)
{
	return join_render_block(list, "\n", 1, "SEPARATOR", 9);
}

void
free_render_blocks(struct render_block *first_block)
{
	struct render_block *head_block = first_block;
	struct render_block *temp;
	while (head_block != NULL) {
		free_wstring(head_block->content);
		free(head_block->content_type);
		temp = head_block;
		head_block = head_block->next;
		free(temp);
	}
}

