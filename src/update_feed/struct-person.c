#include <stdlib.h>
#include "update_feed/update_feed.h"

bool
prepend_person(struct getfeed_person **head_person_ptr)
{
	struct getfeed_person *person = calloc(1, sizeof(struct getfeed_person));
	if (person == NULL) {
		return false;
	}
	person->next = *head_person_ptr;
	*head_person_ptr = person;
	return true;
}

void
reverse_person_list(struct getfeed_person **list)
{
	struct getfeed_person *prev = NULL;
	struct getfeed_person *current = *list;
	struct getfeed_person *next = NULL;
	while (current != NULL) {
		next = current->next;
		current->next = prev;
		prev = current;
		current = next;
	}
	*list = prev;
}

void
free_person(struct getfeed_person *person)
{
	struct getfeed_person *temp;
	struct getfeed_person *p = person;
	while (p != NULL) {
		free_string(p->name);
		free_string(p->email);
		free_string(p->url);
		temp = p;
		p = p->next;
		free(temp);
	}
}

struct string *
generate_person_list_string(const struct getfeed_person *person)
{
	struct string *str = crtes();
	if (str == NULL) {
		return NULL;
	}
	bool added_name, added_email;
	const struct getfeed_person *p = person;
	while (p != NULL) {
		added_name = false;
		if ((p->name != NULL) && (p->name->len != 0)) {
			if (str->len != 0) {
				if (catas(str, ", ", 2) == false) {
					goto error;
				}
			}
			if (catss(str, p->name) == false) {
				goto error;
			}
			added_name = true;
		}
		added_email = false;
		if ((p->email != NULL) && (p->email->len != 0)) {
			if (added_name == true) {
				if (catas(str, " <", 2) == false) {
					goto error;
				}
			}
			if (catss(str, p->email) == false) {
				goto error;
			}
			if (added_name == true) {
				if (catcs(str, '>') == false) {
					goto error;
				}
			}
			added_email = true;
		}
		if ((p->url != NULL) && (p->url->len != 0)) {
			if (added_name == true || added_email == true) {
				if (catas(str, " (", 2) == false) {
					goto error;
				}
			}
			if (catss(str, p->url) == false) {
				goto error;
			}
			if (added_name == true || added_email == true) {
				if (catcs(str, ')') == false) {
					goto error;
				}
			}
		}
		p = p->next;
	}
	return str;
error:
	free_string(str);
	return NULL;
}
