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
		free_string(i->comments_url);
		free_category(i->category);
		free_link(i->attachment);
		free_person(i->author);
		free_person(i->contributor);
		free_string_list(i->location);
		free_picture(i->thumbnail);
		temp = i;
		i = i->next;
		free(temp);
	}
}
