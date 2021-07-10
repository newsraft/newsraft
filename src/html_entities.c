#include <string.h>
#include "feedeater.h"

struct {
	const char *name;
	const char *data;
} entities[] = {
	{"lsquo", "‘"},
	{"rsquo", "’"},
	{"sbquo", "‚"},
	{"ldquo", "“"},
	{"rdquo", "”"},
	{"bdquo", "„"},
	{"hearts", "♥"},
	{"deg", "°"},
	{"trade", "™"},
	{"copy", "©"},
	{"reg", "®"},
};

static const char *
translate_entity(char *entity)
{
	for (size_t i = 0; i < LENGTH(entities); ++i) {
		if (strcmp(entity, entities[i].name) == 0) {
			return entities[i].data;
		}
	}
	return NULL;
}

struct string *
expand_html_entities(char *buf, size_t buf_len)
{
	// multiply by 2 in case if some entities' values greater than their name
	char *text = malloc(sizeof(char) * (buf_len + 1) * 2);
	if (text == NULL) {
		return NULL;
	}
	bool in_entity = false;
	char entity[100], *data;
	size_t i, j, entity_letter;
	for (i = 0, j = 0; i < buf_len; ++i) {
		if (in_entity == true) {
			if (buf[i] == ';') {
				in_entity = false;
				entity[entity_letter] = '\0';
				data = (char *)translate_entity(entity);
				if (data != NULL) {
					text[j] = '\0';
					strcat(text, data);
					j += strlen(data);
				}
			} else {
				entity[entity_letter++] = buf[i];
			}
		} else {
			if (buf[i] == '&') {
				in_entity = true;
				entity_letter = 0;
			} else {
				text[j++] = buf[i];
			}
		}
	}
	text[j] = '\0';
	struct string *text_str = NULL;
	make_string(&text_str, text, j);
	free(text);
	return text_str;
}
