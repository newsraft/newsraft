#include <stdlib.h>
#include "update_feed/parse_feed/parse_feed.h"

static struct getfeed_link *
create_link(void)
{
	struct getfeed_link *link = malloc(sizeof(struct getfeed_link));
	if (link == NULL) {
		return NULL;
	}
	link->url = crtes();
	link->type = crtes();
	link->size = 0;
	link->duration = 0;
	link->next = NULL;
	return link;
}

// On success returns true.
// On memory shortage returns false.
bool
prepend_link(struct getfeed_link **head_link_ptr)
{
	if (*head_link_ptr == NULL) {
		*head_link_ptr = create_link();
	} else {
		struct getfeed_link *link = create_link();
		if (link == NULL) {
			return false;
		}
		link->next = *head_link_ptr;
		*head_link_ptr = link;
	}
	return true;
}

void
free_link(struct getfeed_link *link)
{
	struct getfeed_link *temp;
	struct getfeed_link *l = link;
	while (l != NULL) {
		free_string(l->url);
		free_string(l->type);
		temp = l;
		l = l->next;
		free(temp);
	}
}
