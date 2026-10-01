#include <string.h>
#include "newsraft.h"

struct person {
	struct string *type;
	struct string *name;
	struct string *email;
	struct string *url;
};

static inline void
initialize_person(struct person *p)
{
	p->type = crtes(17);
	p->name = crtes(19);
	p->email = crtes(23);
	p->url = crtes(29);
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

static void
write_person_to_result(struct string *result, const struct person *person)
{
	if (person->type->len == 0 || (person->name->len == 0 && person->email->len == 0 && person->url->len == 0)) {
		return; // Ignore empty persons >,<
	}
	if (result->len > 0) {
		catas(result, ", ", 2);
	}
	if (person->name->len > 0) {
		catss(result, person->name);
	}
	if (person->email->len > 0) {
		if (person->name->len > 0) {
			catas(result, " <", 2);
		}
		catss(result, person->email);
		if (person->name->len > 0) {
			catcs(result, '>');
		}
	}
	if (person->url->len > 0) {
		if (person->name->len > 0 || person->email->len > 0) {
			catas(result, " (", 2);
		}
		catss(result, person->url);
		if (person->name->len > 0 || person->email->len > 0) {
			catcs(result, ')');
		}
	}
	if (person->type->len > 0 && strcmp(person->type->ptr, "author") != 0) {
		if (person->name->len > 0 || person->email->len > 0 || person->url->len > 0) {
			catas(result, " [", 2);
		}
		catss(result, person->type);
		if (person->name->len > 0 || person->email->len > 0 || person->url->len > 0) {
			catcs(result, ']');
		}
	}
}

struct string *
deserialize_persons_string(const char *src)
{
	struct person person;
	struct string *result = crtes(100);
	struct deserialize_stream *stream = open_deserialize_stream(src);
	initialize_person(&person);
	const struct string *field = get_next_entry_from_deserialize_stream(stream);
	while (field != NULL) {
		if (strcmp(field->ptr, "^") == 0) {
			write_person_to_result(result, &person);
			empty_person(&person);
		} else if (strncmp(field->ptr, "type=", 5) == 0) {
			cpyas(&person.type, field->ptr + 5, field->len - 5);
		} else if (strncmp(field->ptr, "name=", 5) == 0) {
			cpyas(&person.name, field->ptr + 5, field->len - 5);
		} else if (strncmp(field->ptr, "email=", 6) == 0) {
			cpyas(&person.email, field->ptr + 6, field->len - 6);
		} else if (strncmp(field->ptr, "url=", 4) == 0) {
			cpyas(&person.url, field->ptr + 4, field->len - 4);
		}
		field = get_next_entry_from_deserialize_stream(stream);
	}
	write_person_to_result(result, &person);
	close_deserialize_stream(stream);
	free_person(&person);
	return result;
}
