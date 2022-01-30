#include "update_feed/update_feed.h"

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
		if (p->name->len != 0) {
			if (str->len != 0) {
				if (catas(str, ", ", 2) == false) {
					goto error;
				}
			}
			if (catas(str, p->name->ptr, p->name->len) == false) {
				goto error;
			}
			added_name = true;
		}
		added_email = false;
		if (p->email->len != 0) {
			if (added_name == true) {
				if (catas(str, " <", 2) == false) {
					goto error;
				}
			}
			if (catas(str, p->email->ptr, p->email->len) == false) {
				goto error;
			}
			if (added_name == true) {
				if (catcs(str, '>') == false) {
					goto error;
				}
			}
			added_email = true;
		}
		if (p->url->len != 0) {
			if (added_name == true || added_email == true) {
				if (catas(str, " (", 2) == false) {
					goto error;
				}
			}
			if (catas(str, p->url->ptr, p->url->len) == false) {
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
