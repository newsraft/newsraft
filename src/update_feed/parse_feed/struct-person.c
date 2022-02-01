#include <stdlib.h>
#include "update_feed/parse_feed/parse_feed.h"

static struct getfeed_person *
create_person(void)
{
	struct getfeed_person *person = malloc(sizeof(struct getfeed_person));
	if (person == NULL) {
		return NULL;
	}
	person->name = crtes();
	person->email = crtes();
	person->url = crtes();
	person->next = NULL;
	return person;
}

bool
prepend_person(struct getfeed_person **head_person_ptr)
{
	if (*head_person_ptr == NULL) {
		*head_person_ptr = create_person();
	} else {
		struct getfeed_person *person = create_person();
		if (person == NULL) {
			return false;
		}
		person->next = *head_person_ptr;
		*head_person_ptr = person;
	}
	return true;
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
