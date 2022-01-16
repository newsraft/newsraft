#include "feedeater.h"

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
	new_entry->next = NULL;

	if (*list != NULL) {
		struct render_block *temp = *list;
		while (temp != NULL) {
			if (temp->next == NULL) {
				temp->next = new_entry;
				break;
			}
			temp = temp->next;
		}
	} else {
		*list = new_entry;
	}
	return true;
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

bool
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
