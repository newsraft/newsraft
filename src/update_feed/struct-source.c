#include <stdlib.h>
#include "update_feed/update_feed.h"

bool
prepend_source(struct getfeed_source **head_ptr)
{
	struct getfeed_source *source = calloc(1, sizeof(struct getfeed_source));
	if (source == NULL) {
		return false;
	}
	source->next = *head_ptr;
	*head_ptr = source;
	return true;
}

void
reverse_source_list(struct getfeed_source **list)
{
	struct getfeed_source *prev = NULL;
	struct getfeed_source *current = *list;
	struct getfeed_source *next = NULL;
	while (current != NULL) {
		next = current->next;
		current->next = prev;
		prev = current;
		current = next;
	}
	*list = prev;
}

void
free_source(struct getfeed_source *source)
{
	struct getfeed_source *temp;
	struct getfeed_source *s = source;
	while (s != NULL) {
		free_string(s->name);
		free_string(s->url);
		temp = s;
		s = s->next;
		free(temp);
	}
}

struct string *
generate_source_list_string(const struct getfeed_source *source)
{
	struct string *str = crtes();
	if (str == NULL) {
		return NULL;
	}
	const struct getfeed_source *s = source;
	while (s != NULL) {
		if ((s->url != NULL) && (s->url->len != 0)) {
			if (str->len != 0) {
				if (catcs(str, '\n') == false) {
					goto error;
				}
			}
			if (catss(str, s->url) == false) {
				goto error;
			}
			if ((s->name != NULL) && (s->name->len !=0)) {
				if (catcs(str, ' ') == false) {
					goto error;
				}
				if (catss(str, s->name) == false) {
					goto error;
				}
			}
		}
		s = s->next;
	}
	return str;
error:
	free_string(str);
	return NULL;
}
