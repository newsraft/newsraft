#include <string.h>
#include "newsraft.h"

struct person {
	struct string *type;
	struct string *name;
	struct string *email;
	struct string *url;
};

static inline bool
initialize_person(struct person *p)
{
	p->type = crtes(17);
	p->name = crtes(19);
	p->email = crtes(23);
	p->url = crtes(29);
	return p->type != NULL && p->name != NULL && p->email != NULL && p->url != NULL;
}

static inline void
empty_person(struct person *p)
{
	empty_string(p->type);
	empty_string(p->name);
	empty_string(p->email);
	empty_string(p->url);
}

static inline void
free_person(struct person *p)
{
	free_string(p->type);
	free_string(p->name);
	free_string(p->email);
	free_string(p->url);
}

static bool
write_person_to_result(struct string *result, const struct person *person)
{
	if ((person->name->len == 0) && (person->email->len == 0) && (person->url->len == 0)) {
		return true; // Ignore empty persons >,<
	}
	if ((result->len != 0) && (catas(result, ", ", 2) == false)) {
		return false;
	}
	if ((person->name->len != 0) && (catss(result, person->name) == false)) {
		return false;
	}
	if (person->email->len != 0) {
		if ((person->name->len != 0) && (catas(result, " <", 2) == false)) {
			return false;
		}
		if (catss(result, person->email) == false) {
			return false;
		}
		if ((person->name->len != 0) && (catcs(result, '>') == false)) {
			return false;
		}
	}
	if (person->url->len != 0) {
		if (((person->name->len != 0) || (person->email->len != 0)) && (catas(result, " (", 2) == false)) {
			return false;
		}
		if (catss(result, person->url) == false) {
			return false;
		}
		if (((person->name->len != 0) || (person->email->len != 0)) && (catcs(result, ')') == false)) {
			return false;
		}
	}
	return true;
}

struct string *
deserialize_persons_string(const char *src, const char *person_type)
{
	struct person person;
	struct string *result = crtes(100);
	struct deserialize_stream *stream = open_deserialize_stream(src);
	if ((initialize_person(&person) == false) || (result == NULL) || (stream == NULL)) {
		goto error;
	}
	const struct string *field = get_next_entry_from_deserialize_stream(stream);
	while (field != NULL) {
		if (strcmp(field->ptr, "^") == 0) {
			if (strcmp(person_type, person.type->ptr) == 0) {
				if (write_person_to_result(result, &person) == false) {
					goto error;
				}
			}
			empty_person(&person);
		} else if (strncmp(field->ptr, "type=", 5) == 0) {
			if (cpyas(&person.type, field->ptr + 5, field->len - 5) == false) {
				goto error;
			}
		} else if (strncmp(field->ptr, "name=", 5) == 0) {
			if (cpyas(&person.name, field->ptr + 5, field->len - 5) == false) {
				goto error;
			}
		} else if (strncmp(field->ptr, "email=", 6) == 0) {
			if (cpyas(&person.email, field->ptr + 6, field->len - 6) == false) {
				goto error;
			}
		} else if (strncmp(field->ptr, "url=", 4) == 0) {
			if (cpyas(&person.url, field->ptr + 4, field->len - 4) == false) {
				goto error;
			}
		}
		field = get_next_entry_from_deserialize_stream(stream);
	}
	if (write_person_to_result(result, &person) == false) {
		goto error;
	}
	close_deserialize_stream(stream);
	free_person(&person);
	return result;
error:
	close_deserialize_stream(stream);
	free_person(&person);
	free_string(result);
	return NULL;
}
