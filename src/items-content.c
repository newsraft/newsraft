#include "feedeater.h"

int
append_content(struct content_list **list, const char *content, size_t content_len, const char *content_type, size_t content_type_len)
{
	struct content_list *new_entry = malloc(sizeof(struct content_list));
	if (new_entry == NULL) {
		return 1;
	}
	new_entry->content = create_string(content, content_len);
	if (new_entry->content == NULL) {
		free(new_entry);
		return 1;
	}
	new_entry->content_type = malloc(sizeof(char) * (content_type_len + 1));
	if (new_entry->content_type == NULL) {
		free_string(new_entry->content);
		free(new_entry);
		return 1;
	}
	strncpy(new_entry->content_type, content_type, content_type_len);
	new_entry->content_type[content_type_len] = '\0';
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
	return 0;
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
append_links_of_item_to_its_contents(struct content_list **contents, struct trim_link_list *links)
{
	struct string *str = create_string("\nLinks:\n", 8);
	if (str == NULL) {
		return false;
	}
	// Square brackets, colon and space (4) + longest size_t (20) + terminator (1)
#define LINK_PREFIX_SIZE 25
	char word[LINK_PREFIX_SIZE];
	size_t word_len;
	bool not_first_link = false;
	bool appended_type;
	for (size_t i = 0; i < links->len; ++i) {
		if ((links->list[i].url != NULL) && (links->list[i].url->len != 0)) {
			if (not_first_link == true) {
				if (catcs(str, '\n') == false) {
					goto error;
				}
			}
			not_first_link = true;
			word_len = snprintf(word, LINK_PREFIX_SIZE, "[%zu]: ", i + 1);
#undef LINK_PREFIX_SIZE
			if (catas(str, word, word_len) == false) {
				goto error;
			}
			if (catss(str, links->list[i].url) == false) {
				goto error;
			}
			appended_type = false;
			if ((links->list[i].type != NULL) && (links->list[i].type->len != 0)) {
				if (catas(str, " (type: ", 8) == false) {
					goto error;
				}
				if (catss(str, links->list[i].type) == false) {
					goto error;
				}
				appended_type = true;
			}
			if (appended_type == true) {
				if (catcs(str, ')') == false) {
					goto error;
				}
			}
		}
	}
	if (append_content(contents, str->ptr, str->len, "text/plain", 10) != 0) {
		goto error;
	}
	free_string(str);
	return true;
error:
	free_string(str);
	return false;
}
