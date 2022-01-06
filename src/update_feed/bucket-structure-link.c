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
			links->list[links->len].size = NULL;
			links->list[links->len].duration = NULL;
			return false;
		}
		if ((links->list[links->len].type = create_empty_string()) == NULL) {
			FAIL("Not enough memory for link type string.");
			links->list[links->len].size = NULL;
			links->list[links->len].duration = NULL;
			return false;
		}
		if ((links->list[links->len].size = create_empty_string()) == NULL) {
			FAIL("Not enough memory for link size string.");
			links->list[links->len].duration = NULL;
			return false;
		}
		if ((links->list[links->len].duration = create_empty_string()) == NULL) {
			FAIL("Not enough memory for link duration string.");
			return false;
		}
	} else {
		empty_string(links->list[links->len].url);
		empty_string(links->list[links->len].type);
		empty_string(links->list[links->len].size);
		empty_string(links->list[links->len].duration);
	}
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

bool
add_size_to_last_link(const struct link_list *links, const char *value, size_t value_len)
{
	return cpyas(links->list[links->len - 1].size, value, value_len);
}

bool
add_duration_to_last_link(const struct link_list *links, const char *value, size_t value_len)
{
	return cpyas(links->list[links->len - 1].duration, value, value_len);
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
		free_string(links->list[i].size);
		free_string(links->list[i].duration);
	}
	free(links->list);
}

struct string *
generate_link_list_string_for_database(const struct link_list *links)
{
	struct string *str = create_empty_string();
	if (str == NULL) {
		return NULL;
	}
	bool is_this_first_link = true;
	for (size_t i = 0; i < links->len; ++i) {
		if (links->list[i].url->len != 0) {
			if (is_this_first_link == false) {
				if (catcs(str, '\n') == false) { goto error; }
			}
			strip_whitespace_from_string(links->list[i].url);
			if (catss(str, links->list[i].url) == false) { goto error; }
			is_this_first_link = false;
		} else {
			continue;
		}
		if (catcs(str, ' ') == false) { goto error; }
		if (links->list[i].type->len != 0) {
			strip_whitespace_from_string(links->list[i].type);
			if (catss(str, links->list[i].type) == false) { goto error; }
		}
		if (catcs(str, ' ') == false) { goto error; }
		if (links->list[i].size->len != 0) {
			strip_whitespace_from_string(links->list[i].size);
			if (catss(str, links->list[i].size) == false) { goto error; }
		}
		if (catcs(str, ' ') == false) { goto error; }
		if (links->list[i].duration->len != 0) {
			strip_whitespace_from_string(links->list[i].duration);
			if (catss(str, links->list[i].duration) == false) { goto error; }
		}
	}
	return str;
error:
	free_string(str);
	return NULL;
}
