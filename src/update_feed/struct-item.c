#include <string.h>
#include "update_feed/parse_feed/parse_feed.h"

// On success returns true.
// On memory shortage returns false.
static struct getfeed_item *
create_item(void)
{
	struct getfeed_item *item = malloc(sizeof(struct getfeed_item));
	if (item == NULL)                           { return NULL; }
	if ((item->guid          = crtes()) == NULL) { goto undo0; }
	if ((item->title.value   = crtes()) == NULL) { goto undo1; }
	if ((item->title.type    = crtes()) == NULL) { goto undo2; }
	if ((item->url           = crtes()) == NULL) { goto undo3; }
	if ((item->summary.value = crtes()) == NULL) { goto undo4; }
	if ((item->summary.type  = crtes()) == NULL) { goto undo5; }
	if ((item->content.value = crtes()) == NULL) { goto undo6; }
	if ((item->content.type  = crtes()) == NULL) { goto undo7; }
	if ((item->comments_url  = crtes()) == NULL) { goto undo8; }
	item->category = NULL;
	item->attachment = NULL;
	item->author = NULL;
	item->contributor = NULL;
	item->pubdate = 0;
	item->upddate = 0;
	item->next = NULL;
	return item;
undo8:
	free_string(item->content.type);
undo7:
	free_string(item->content.value);
undo6:
	free_string(item->summary.type);
undo5:
	free_string(item->summary.value);
undo4:
	free_string(item->url);
undo3:
	free_string(item->title.type);
undo2:
	free_string(item->title.value);
undo1:
	free_string(item->guid);
undo0:
	free(item);
	return NULL;
}

bool
prepend_item(struct getfeed_item **head_item_ptr)
{
	struct getfeed_item *item = create_item();
	if (item == NULL) {
		return false;
	}
	if (*head_item_ptr != NULL) {
		item->next = *head_item_ptr;
	}
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
		temp = i;
		i = i->next;
		free(temp);
	}
}
