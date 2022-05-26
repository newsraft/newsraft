#include <stdlib.h>
#include "update_feed/update_feed.h"

// On success returns true.
// On memory shortage returns false.
bool
prepend_link(struct getfeed_link **head_link_ptr)
{
	struct getfeed_link *link = calloc(1, sizeof(struct getfeed_link));
	if (link == NULL) {
		return false;
	}
	link->next = *head_link_ptr;
	*head_link_ptr = link;
	return true;
}

void
reverse_link_list(struct getfeed_link **list)
{
	struct getfeed_link *prev = NULL;
	struct getfeed_link *current = *list;
	struct getfeed_link *next = NULL;
	while (current != NULL) {
		next = current->next;
		current->next = prev;
		prev = current;
		current = next;
	}
	*list = prev;
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
	struct string *str = crtes();
	if (str == NULL) {
		return NULL;
	}
	char tmp[100];
	int64_t tmp_len;
	const struct getfeed_link *l = link;
	while (l != NULL) {
		if ((l->url == NULL) || (l->url->len == 0)) {
			continue;
		}
		if (str->len != 0) {
			if (catcs(str, '\n') == false) { goto error; }
		}
		if (catss(str, l->url) == false) { goto error; }
		if (catcs(str, ' ') == false) { goto error; }
		if ((l->type != NULL) && (l->type->len != 0)) {
			if (catss(str, l->type) == false) { goto error; }
		}
		// Mind the heading space character! It's needed for separation.
		tmp_len = snprintf(tmp, 100, " %zu", l->size);
		if ((tmp_len < 0) || (tmp_len >= 100)) { goto error; }
		if (catas(str, tmp, tmp_len) == false) { goto error; }
		tmp_len = snprintf(tmp, 100, " %zu", l->duration);
		if ((tmp_len < 0) || (tmp_len >= 100)) { goto error; }
		if (catas(str, tmp, tmp_len) == false) { goto error; }
		l = l->next;
	}
	return str;
error:
	free_string(str);
	return NULL;
}
