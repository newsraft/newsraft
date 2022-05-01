#include <stdlib.h>
#include <string.h>
#include "update_feed/update_feed.h"

static inline struct getfeed_link *
create_link(void)
{
	struct getfeed_link *link = malloc(sizeof(struct getfeed_link));
	if (link == NULL) {
		return NULL;
	}
	link->url = crtes();
	link->type = crtes();
	if ((link->url == NULL) || (link->type == NULL)) {
		free_string(link->url);
		free_string(link->type);
		free(link);
		return NULL;
	}
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
	struct getfeed_link *link = create_link();
	if (link == NULL) {
		return false;
	}
	link->next = *head_link_ptr;
	*head_link_ptr = link;
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

struct string *
generate_link_list_string(const struct getfeed_link *link)
{
	char temp[123];
	struct string *str = crtes();
	if (str == NULL) {
		return NULL;
	}
	const struct getfeed_link *l = link;
	while (l != NULL) {
		if (l->url->len != 0) {
			if (str->len != 0) {
				if (catcs(str, '\n') == false) { goto error; }
			}
			if (catas(str, l->url->ptr, l->url->len) == false) { goto error; }
		} else {
			continue;
		}
		if (catcs(str, ' ') == false) { goto error; }
		if (l->type->len != 0) {
			if (catas(str, l->type->ptr, l->type->len) == false) { goto error; }
		}
		// Mind the heading space character! It is needed for separation.
		snprintf(temp, 123, " %zu", l->size);
		if (catas(str, temp, strlen(temp)) == false) { goto error; }
		snprintf(temp, 123, " %zu", l->duration);
		if (catas(str, temp, strlen(temp)) == false) { goto error; }
		l = l->next;
	}
	return str;
error:
	free_string(str);
	return NULL;
}
