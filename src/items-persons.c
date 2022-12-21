#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

struct person {
	struct string *type;
	struct string *name;
	struct string *email;
	struct string *url;
};

static struct person *
create_person(void)
{
	struct person *person = malloc(sizeof(struct person));
	if (person == NULL) {
		return NULL;
	}
	person->type = crtes();
	person->name = crtes();
	person->email = crtes();
	person->url = crtes();
	if ((person->type == NULL)
		|| (person->name == NULL)
		|| (person->email == NULL)
		|| (person->url == NULL))
	{
		free_string(person->type);
		free_string(person->name);
		free_string(person->email);
		free_string(person->url);
		free(person);
		return NULL;
	}
	return person;
}

static void
empty_person(struct person *person)
{
	empty_string(person->type);
	empty_string(person->name);
	empty_string(person->email);
	empty_string(person->url);
}

static void
free_person(struct person *person)
{
	if (person != NULL) {
		free_string(person->type);
		free_string(person->name);
		free_string(person->email);
		free_string(person->url);
		free(person);
	}
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
	struct string *result = crtes();
	struct person *person = create_person();
	struct deserialize_stream *stream = open_deserialize_stream(src);
	if ((person == NULL) || (result == NULL) || (stream == NULL)) {
		goto error;
	}
	const struct string *field = get_next_entry_from_deserialize_stream(stream);
	while (field != NULL) {
		if (strcmp(field->ptr, "^") == 0) {
			if (strcmp(person_type, person->type->ptr) == 0) {
				if (write_person_to_result(result, person) == false) {
					goto error;
				}
			}
			empty_person(person);
		} else if (strncmp(field->ptr, "type=", 5) == 0) {
			if (cpyas(person->type, field->ptr + 5, field->len - 5) == false) {
				goto error;
			}
		} else if (strncmp(field->ptr, "name=", 5) == 0) {
			if (cpyas(person->name, field->ptr + 5, field->len - 5) == false) {
				goto error;
			}
		} else if (strncmp(field->ptr, "email=", 6) == 0) {
			if (cpyas(person->email, field->ptr + 6, field->len - 6) == false) {
				goto error;
			}
		} else if (strncmp(field->ptr, "url=", 4) == 0) {
			if (cpyas(person->url, field->ptr + 4, field->len - 4) == false) {
				goto error;
			}
		}
		field = get_next_entry_from_deserialize_stream(stream);
	}
	if (write_person_to_result(result, person) == false) {
		goto error;
	}
	close_deserialize_stream(stream);
	free_person(person);
	return result;
error:
	close_deserialize_stream(stream);
	free_person(person);
	free_string(result);
	return NULL;
}
