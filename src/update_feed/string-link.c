#include "update_feed/update_feed.h"

struct string *
generate_link_list_string(const struct getfeed_link *link)
{
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
			if (catss(str, (struct string *)l->url) == false) { goto error; }
		} else {
			continue;
		}
		if (catcs(str, ' ') == false) { goto error; }
		if (l->type->len != 0) {
			if (catss(str, (struct string *)l->type) == false) { goto error; }
		}
		if (catcs(str, ' ') == false) { goto error; }
		if (l->size->len != 0) {
			if (catss(str, (struct string *)l->size) == false) { goto error; }
		}
		if (catcs(str, ' ') == false) { goto error; }
		if (l->duration->len != 0) {
			if (catss(str, (struct string *)l->duration) == false) { goto error; }
		}
		l = l->next;
	}
	return str;
error:
	free_string(str);
	return NULL;
}
