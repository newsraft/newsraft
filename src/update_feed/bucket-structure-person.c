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
		if ((persons->list[persons->len].name = create_empty_string()) == NULL) {
			FAIL("Not enough memory for person name string.");
			persons->list[persons->len].email = NULL;
			persons->list[persons->len].link = NULL;
			return false;
		}
		if ((persons->list[persons->len].email = create_empty_string()) == NULL) {
			FAIL("Not enough memory for person email string.");
			persons->list[persons->len].link = NULL;
			return false;
		}
		if ((persons->list[persons->len].link = create_empty_string()) == NULL) {
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

int
add_name_to_last_person(struct person_list *persons, const struct string *value)
{
	return cpyss(persons->list[persons->len - 1].name, value);
}

int
add_email_to_last_person(struct person_list *persons, const struct string *value)
{
	return cpyss(persons->list[persons->len - 1].email, value);
}

int
add_link_to_last_person(struct person_list *persons, const struct string *value)
{
	return cpyss(persons->list[persons->len - 1].link, value);
}

void
empty_person_list(struct person_list *persons)
{
	persons->len = 0;
}

void
free_person_list(struct person_list *persons)
{
	for (size_t i = 0; i < persons->lim; ++i) {
		free_string(persons->list[i].name);
		free_string(persons->list[i].email);
		free_string(persons->list[i].link);
	}
	free(persons->list);
	// WARNING: Do not free memory under persons pointer because we store person list structures in stack memory.
}

struct string *
generate_person_list_string(const struct person_list *persons)
{
	struct string *str = create_empty_string();
	if (str == NULL) {
		return NULL;
	}
	bool added_name, added_email, added_link;
	for (size_t i = 0; i < persons->len; ++i) {
		added_name = false;
		added_email = false;
		added_link = false;
		if (persons->list[i].name->len != 0) {
			catss(str, persons->list[i].name);
			added_name = true;
		}
		if (persons->list[i].email->len != 0) {
			if (added_name == true) {
				catas(str, " <", 2);
			}
			catss(str, persons->list[i].email);
			if (added_name == true) {
				catcs(str, '>');
			}
			added_email = true;
		}
		if (persons->list[i].link->len != 0) {
			if (added_name == true || added_email == true) {
				catas(str, " (", 2);
			}
			catss(str, persons->list[i].link);
			if (added_name == true || added_email == true) {
				catcs(str, ')');
			}
			added_link = true;
		}
		if ((i + 1 != persons->len) && (added_name == true || added_email == true || added_link == true)) {
			catas(str, ", ", 2);
		}
	}
	return str;
}
