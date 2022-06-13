#include "update_feed/update_feed.h"

void
empty_category(struct getfeed_temp *temp)
{
	empty_string_safe(temp->category.term);
	empty_string_safe(temp->category.scheme);
	empty_string_safe(temp->category.label);
}

bool
serialize_category(struct getfeed_temp *temp, struct string **str)
{
	if ((temp->category.term == NULL) || (temp->category.term->len == 0)) {
		return true;
	}
	if (cat_string_to_serialization(str, "term", 4, temp->category.term) == false) {
		return false;
	}
	if (cat_string_to_serialization(str, "scheme", 6, temp->category.scheme) == false) {
		return false;
	}
	if (cat_string_to_serialization(str, "label", 5, temp->category.label) == false) {
		return false;
	}
	return true;
}
