#include <stdlib.h>
#include "update_feed/update_feed.h"

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
	for (struct getfeed_item *i = item; i != NULL; item = i) {
		free_string(item->guid);
		free_string(item->title);
		free_string(item->url);
		free_string(item->content);
		free_string(item->attachments);
		free_string(item->persons);
		free_string(item->extras);
		i = item->next;
		free(item);
	}
}
