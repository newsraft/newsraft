#include "update_feed/update_feed.h"

void
empty_person(struct getfeed_temp *temp)
{
	temp->author.involvement = PERSON_AUTHOR;
	empty_string_safe(temp->author.name);
	empty_string_safe(temp->author.email);
	empty_string_safe(temp->author.url);
}

bool
serialize_person(struct getfeed_temp *temp, struct string **str)
{
	if (((temp->author.name == NULL) || (temp->author.name->len == 0))
			&& ((temp->author.email == NULL) || (temp->author.email->len == 0))
			&& ((temp->author.url == NULL) || (temp->author.url->len == 0)))
	{
		return true;
	}
	if (temp->author.involvement == PERSON_AUTHOR) {
		if (cat_array_to_serialization(str, "type", 4, "author", 6) == false) {
			return false;
		}
	} else if (temp->author.involvement == PERSON_CONTRIBUTOR) {
		if (cat_array_to_serialization(str, "type", 4, "contributor", 11) == false) {
			return false;
		}
	} else if (temp->author.involvement == PERSON_EDITOR) {
		if (cat_array_to_serialization(str, "type", 4, "editor", 6) == false) {
			return false;
		}
	} else if (temp->author.involvement == PERSON_WEBMASTER) {
		if (cat_array_to_serialization(str, "type", 4, "webmaster", 9) == false) {
			return false;
		}
	}
	if (cat_string_to_serialization(str, "name", 4, temp->author.name) == false) {
		return false;
	}
	if (cat_string_to_serialization(str, "email", 5, temp->author.email) == false) {
		return false;
	}
	if (cat_string_to_serialization(str, "url", 3, temp->author.url) == false) {
		return false;
	}
	return true;
}
