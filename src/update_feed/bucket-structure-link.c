#include <stdlib.h>
#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

void
initialize_link_list(struct link_list *links)
{
	links->list = NULL;
	links->len = 0;
	links->lim = 0;
}

// On success returns true.
// On memory shortage returns false.
bool
expand_link_list_by_one_element(struct link_list *links)
{
	if (links->len == links->lim) {
		struct link *new_list = realloc(links->list, sizeof(struct link) * (links->lim + 1));
		if (new_list == NULL) {
			FAIL("Not enough memory for new link!");
			return false;
		}
		links->list = new_list;
		++(links->lim);
		if ((links->list[links->len].url = create_empty_string()) == NULL) {
			FAIL("Not enough memory for link url string.");
			links->list[links->len].type = NULL;
			return false;
		}
		if ((links->list[links->len].type = create_empty_string()) == NULL) {
			FAIL("Not enough memory for link type string.");
			return false;
		}
	} else {
		empty_string(links->list[links->len].url);
		empty_string(links->list[links->len].type);
	}
	links->list[links->len].size = 0;
	links->list[links->len].duration = 0;
	++(links->len);
	return true;
}

bool
add_url_to_last_link(const struct link_list *links, const char *value, size_t value_len)
{
	return cpyas(links->list[links->len - 1].url, value, value_len);
}

bool
add_type_to_last_link(const struct link_list *links, const char *value, size_t value_len)
{
	return cpyas(links->list[links->len - 1].type, value, value_len);
}

// On success returns 0.
// On failure returns non-zero.
bool
add_size_to_last_link(const struct link_list *links, const char *value)
{
	int size;
	if (sscanf(value, "%d", &size) == 1) {
		links->list[links->len - 1].size = size;
		return true;
	}
	return false;
}

void
empty_link_list(struct link_list *links)
{
	links->len = 0;
}

void
free_link_list(const struct link_list *links)
{
	for (size_t i = 0; i < links->lim; ++i) {
		free_string(links->list[i].url);
		free_string(links->list[i].type);
	}
	free(links->list);
}

struct string *
generate_link_list_string(const struct link_list *links)
{
	struct string *str = create_empty_string();
	if (str == NULL) {
		return NULL;
	}
	struct string *readable_size;
	bool added_url = false;
	for (size_t i = 0; i < links->len; ++i) {
		if (links->list[i].url->len != 0) {
			if (added_url == true) {
				catcs(str, '\n');
			}
			catss(str, links->list[i].url);
			added_url = true;
		} else {
			continue;
		}
		catcs(str, ' ');
		if (links->list[i].type->len != 0) {
			catss(str, links->list[i].type);
		}
		catcs(str, ' ');
		if (links->list[i].size != 0) {
			readable_size = convert_bytes_to_human_readable_size_string(links->list[i].size);
			if (readable_size != NULL) {
				catss(str, readable_size);
				free_string(readable_size);
			} else {
				goto error;
			}
		}
	}
	return str;
error:
	free_string(str);
	return NULL;
}
