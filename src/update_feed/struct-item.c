#include <stdlib.h>
#include "update_feed/parse_feed/parse_feed.h"

bool
prepend_item(struct getfeed_item **head_item_ptr)
{
	struct getfeed_item *item = calloc(1, sizeof(struct getfeed_item));
	if (item == NULL) {
		return false;
	}
	item->next = *head_item_ptr;
	*head_item_ptr = item;
	return true;
}

void
free_item(struct getfeed_item *item)
{
	struct getfeed_item *temp;
	struct getfeed_item *i = item;
	while (i != NULL) {
		free_string(i->guid);
		free_string(i->title.value);
		free_string(i->title.type);
		free_string(i->url);
		free_string(i->summary.value);
		free_string(i->summary.type);
		free_string(i->content.value);
		free_string(i->content.type);
		free_string(i->attachments);
		free_string(i->sources);
		free_string(i->authors);
		free_string(i->comments_url);
		free_string(i->locations);
		free_string(i->categories);
		free_string(i->language);
		free_string(i->rights.value);
		free_string(i->rights.type);
		free_string(i->rating);
		free_string(i->pictures);
		temp = i;
		i = i->next;
		free(temp);
	}
}
