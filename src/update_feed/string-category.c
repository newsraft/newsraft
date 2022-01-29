#include "update_feed/update_feed.h"

struct string *
generate_category_list_string(const struct getfeed_category *category)
{
	struct string *str = crtes();
	if (str == NULL) {
		return NULL;
	}
	const struct getfeed_category *c = category;
	while (c != NULL) {
		if ((c->term->len == 0) && (c->label->len == 0)) {
			continue;
		}
		if (str->len != 0) {
			if (catas(str, "; ", 2) == false) {
				goto error;
			}
		}
		if (c->label->len != 0) {
			if (catas(str, c->label->ptr, c->label->len) == false) {
				goto error;
			}
		} else {
			if (catas(str, c->term->ptr, c->term->len) == false) {
				goto error;
			}
		}
		c = c->next;
	}
	return str;
error:
	free_string(str);
	return NULL;
}
