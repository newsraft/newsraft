#include <string.h>
#include "update_feed/update_feed.h"

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
