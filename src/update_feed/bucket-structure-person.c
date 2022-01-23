#include <stdlib.h>
#include <string.h>
#include "update_feed/update_feed.h"

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
