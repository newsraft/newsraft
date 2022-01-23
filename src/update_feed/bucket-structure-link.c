#include <stdlib.h>
#include <string.h>
#include "update_feed/update_feed.h"

struct string *
generate_link_list_string_for_database(const struct link_list *links)
{
	struct string *str = crtes();
	if (str == NULL) {
		return NULL;
	}
	bool is_this_first_link = true;
	for (size_t i = 0; i < links->len; ++i) {
		if (links->list[i].url->len != 0) {
			if (is_this_first_link == false) {
				if (catcs(str, '\n') == false) { goto error; }
			}
			trim_whitespace_from_string(links->list[i].url);
			if (catss(str, links->list[i].url) == false) { goto error; }
			is_this_first_link = false;
		} else {
			continue;
		}
		if (catcs(str, ' ') == false) { goto error; }
		if (links->list[i].type->len != 0) {
			trim_whitespace_from_string(links->list[i].type);
			if (catss(str, links->list[i].type) == false) { goto error; }
		}
		if (catcs(str, ' ') == false) { goto error; }
		if (links->list[i].size->len != 0) {
			trim_whitespace_from_string(links->list[i].size);
			if (catss(str, links->list[i].size) == false) { goto error; }
		}
		if (catcs(str, ' ') == false) { goto error; }
		if (links->list[i].duration->len != 0) {
			trim_whitespace_from_string(links->list[i].duration);
			if (catss(str, links->list[i].duration) == false) { goto error; }
		}
	}
	return str;
error:
	free_string(str);
	return NULL;
}
