#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

struct {
	const char *name;
	const char *value;
} entities[] = {
	{"lt", "<"},
	{"gt", ">"},
	{"amp", "&"},
	{"apos", "'"},
	{"quot", "\""},
};

static char *
get_entity_value(char *name)
{
	for (size_t i = 0; i < LENGTH(entities); ++i) {
		if (strcmp(name, entities[i].name) == 0) {
			return (char *)entities[i].value;
		}
	}
	return NULL;
}

char *
expand_xml_entities(char *buff, size_t buff_len)
{
	char *text = malloc(sizeof(char) * (buff_len + 1));
	if (text == NULL) {
		return NULL;
	}
	char entity_name[100], *entity;
	bool in_entity = false;
	size_t i, j, entity_letter;
	for (i = 0, j = 0; i < buff_len; ++i) {
		if (in_entity == true) {
			if (buff[i] == ';') {
				in_entity = false;
				entity_name[entity_letter] = '\0';
				entity = get_entity_value(entity_name);
				if (entity != NULL) {
					text[j] = '\0';
					strcat(text, entity);
					j += strlen(entity);
				}
			} else {
				entity_name[entity_letter++] = buff[i];
			}
		} else {
			if (buff[i] == '&') {
				in_entity = true;
				entity_letter = 0;
			} else {
				text[j++] = buff[i];
			}
		}
	}
	text[j] = '\0';
	return text;
}
