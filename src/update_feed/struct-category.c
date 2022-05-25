#include <stdlib.h>
#include "update_feed/update_feed.h"

static inline struct getfeed_category *
create_category(void)
{
	struct getfeed_category *category = malloc(sizeof(struct getfeed_category));
	if (category == NULL) {
		return NULL;
	}
	category->label = crtes();
	category->term = crtes();
	category->scheme = crtes();
	if ((category->label == NULL) || (category->term == NULL) || (category->scheme == NULL)) {
		free_string(category->label);
		free_string(category->term);
		free_string(category->scheme);
		free(category);
		return NULL;
	}
	category->next = NULL;
	return category;
}

bool
prepend_category(struct getfeed_category **head_category_ptr)
{
	struct getfeed_category *category = create_category();
	if (category == NULL) {
		return false;
	}
	category->next = *head_category_ptr;
	*head_category_ptr = category;
	return true;
}

void
reverse_category_list(struct getfeed_category **list)
{
	struct getfeed_category *prev = NULL;
	struct getfeed_category *current = *list;
	struct getfeed_category *next = NULL;
	while (current != NULL) {
		next = current->next;
		current->next = prev;
		prev = current;
		current = next;
	}
	*list = prev;
}

void
free_category(struct getfeed_category *category)
{
	struct getfeed_category *temp;
	struct getfeed_category *c = category;
	while (c != NULL) {
		free_string(c->label);
		free_string(c->term);
		free_string(c->scheme);
		temp = c;
		c = c->next;
		free(temp);
	}
}

struct string *
generate_category_list_string(const struct getfeed_category *category)
{
	struct string *str = crtes();
	if (str == NULL) {
		return NULL;
	}
	const struct getfeed_category *c = category;
	while (c != NULL) {
		if ((c->term->len == 0) && (c->label->len == 0)) {
			continue;
		}
		if (str->len != 0) {
			if (catas(str, "; ", 2) == false) {
				goto error;
			}
		}
		if (c->label->len != 0) {
			if (catas(str, c->label->ptr, c->label->len) == false) {
				goto error;
			}
		} else {
			if (catas(str, c->term->ptr, c->term->len) == false) {
				goto error;
			}
		}
		c = c->next;
	}
	return str;
error:
	free_string(str);
	return NULL;
}
