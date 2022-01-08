#include <stdlib.h>
#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

void
initialize_person_list(struct person_list *persons)
{
	persons->list = NULL;
	persons->len = 0;
	persons->lim = 0;
}

// On success returns true.
// On memory shortage returns false.
bool
expand_person_list_by_one_element(struct person_list *persons)
{
	if (persons->len == persons->lim) {
		struct person *new_list = realloc(persons->list, sizeof(struct person) * (persons->lim + 1));
		if (new_list == NULL) {
			FAIL("Not enough memory for new person!");
			return false;
		}
		persons->list = new_list;
		++(persons->lim);
		if ((persons->list[persons->len].name = crtes()) == NULL) {
			FAIL("Not enough memory for person name string.");
			persons->list[persons->len].email = NULL;
			persons->list[persons->len].link = NULL;
			return false;
		}
		if ((persons->list[persons->len].email = crtes()) == NULL) {
			FAIL("Not enough memory for person email string.");
			persons->list[persons->len].link = NULL;
			return false;
		}
		if ((persons->list[persons->len].link = crtes()) == NULL) {
			FAIL("Not enough memory for person link string.");
			return false;
		}
	} else {
		empty_string(persons->list[persons->len].name);
		empty_string(persons->list[persons->len].email);
		empty_string(persons->list[persons->len].link);
	}
	++(persons->len);
	return true;
}

bool
add_name_to_last_person(const struct person_list *persons, const struct string *value)
{
	return cpyss(persons->list[persons->len - 1].name, value);
}

bool
add_email_to_last_person(const struct person_list *persons, const struct string *value)
{
	return cpyss(persons->list[persons->len - 1].email, value);
}

bool
add_link_to_last_person(const struct person_list *persons, const struct string *value)
{
	return cpyss(persons->list[persons->len - 1].link, value);
}

void
empty_person_list(struct person_list *persons)
{
	persons->len = 0;
}

void
free_person_list(const struct person_list *persons)
{
	for (size_t i = 0; i < persons->lim; ++i) {
		free_string(persons->list[i].name);
		free_string(persons->list[i].email);
		free_string(persons->list[i].link);
	}
	free(persons->list);
}

struct string *
generate_person_list_string(const struct person_list *persons)
{
	struct string *str = crtes();
	if (str == NULL) {
		return NULL;
	}
	bool added_name, added_email, added_link;
	for (size_t i = 0; i < persons->len; ++i) {
		added_name = false;
		added_email = false;
		added_link = false;
		if (persons->list[i].name->len != 0) {
			if (catss(str, persons->list[i].name) == false) {
				goto error;
			}
			added_name = true;
		}
		if (persons->list[i].email->len != 0) {
			if (added_name == true) {
				if (catas(str, " <", 2) == false) {
					goto error;
				}
			}
			if (catss(str, persons->list[i].email) == false) {
				goto error;
			}
			if (added_name == true) {
				if (catcs(str, '>') == false) {
					goto error;
				}
			}
			added_email = true;
		}
		if (persons->list[i].link->len != 0) {
			if (added_name == true || added_email == true) {
				if (catas(str, " (", 2) == false) {
					goto error;
				}
			}
			if (catss(str, persons->list[i].link) == false) {
				goto error;
			}
			if (added_name == true || added_email == true) {
				if (catcs(str, ')') == false) {
					goto error;
				}
			}
			added_link = true;
		}
		if ((i + 1 != persons->len) && (added_name == true || added_email == true || added_link == true)) {
			if (catas(str, ", ", 2) == false) {
				goto error;
			}
		}
	}
	return str;
error:
	free_string(str);
	return NULL;
}
