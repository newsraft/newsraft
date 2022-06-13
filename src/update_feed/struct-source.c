#include "update_feed/update_feed.h"

void
empty_source(struct getfeed_temp *temp)
{
	empty_string_safe(temp->source.url);
	empty_string_safe(temp->source.title);
}

bool
serialize_source(struct getfeed_temp *temp, struct string **str)
{
	if ((temp->source.url == NULL) || (temp->source.url->len == 0)) {
		return true;
	}
	if (cat_string_to_serialization(str, "url", 3, temp->source.url) == false) {
		return false;
	}
	if (cat_string_to_serialization(str, "title", 5, temp->source.title) == false) {
		return false;
	}
	return true;
}
