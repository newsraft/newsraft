#include "feedeater.h"

bool
append_content(struct content_list **list, const char *content, size_t content_len, const char *content_type, size_t content_type_len, bool trim_whitespace)
{
	struct content_list *new_entry = malloc(sizeof(struct content_list));
	if (new_entry == NULL) {
		return false;
	}
	new_entry->content = create_string(content, content_len);
	if (new_entry->content == NULL) {
		free(new_entry);
		return false;
	}
	new_entry->content_type = malloc(sizeof(char) * (content_type_len + 1));
	if (new_entry->content_type == NULL) {
		free_string(new_entry->content);
		free(new_entry);
		return false;
	}
	strncpy(new_entry->content_type, content_type, content_type_len);
	new_entry->content_type[content_type_len] = '\0';
	new_entry->trim_whitespace = trim_whitespace;
	new_entry->next = NULL;

	if (*list != NULL) {
		struct content_list *temp = *list;
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

void
free_content_list(struct content_list *list)
{
	struct content_list *head_content = list;
	struct content_list *temp;
	while (head_content != NULL) {
		free_string(head_content->content);
		free(head_content->content_type);
		temp = head_content;
		head_content = head_content->next;
		free(temp);
	}
}

bool
append_links_to_contents(struct content_list **contents, struct link_list *links)
{
	struct string *str = generate_link_list_string_for_pager(links);
	if (str == NULL) {
		return false;
	}
	bool status = append_content(contents, str->ptr, str->len, "text/plain", 10, false);
	free_string(str);
	return status;
}
