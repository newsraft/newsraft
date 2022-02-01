#include <stdlib.h>
#include "update_feed/parse_feed/parse_feed.h"

static struct getfeed_category *
create_category(void)
{
	struct getfeed_category *category = malloc(sizeof(struct getfeed_category));
	if (category == NULL) {
		return NULL;
	}
	category->label = crtes();
	category->term = crtes();
	category->scheme = crtes();
	category->next = NULL;
	return category;
}

bool
prepend_category(struct getfeed_category **head_category_ptr)
{
	if (*head_category_ptr == NULL) {
		*head_category_ptr = create_category();
		if (*head_category_ptr == NULL) {
			return false;
		}
	} else {
		struct getfeed_category *category = create_category();
		if (category == NULL) {
			return false;
		}
		category->next = *head_category_ptr;
		*head_category_ptr = category;
	}
	return true;
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
