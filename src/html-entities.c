#include <string.h>
#include "feedeater.h"

// This has to be the length of the longest entity name in entities array.
#define MAX_ENTITY_NAME_LENGTH 7

struct entity_entry {
	const char *name;
	const char *value;
};

// WARNING!
// Before you add here an entity whose value is longer than name,
// you have to make the data buffer dynamic to avoid overflow!
// Also don't forget to update MAX_ENTITY_NAME_LENGTH every time
// you add another entity with the longest name.
static const struct entity_entry entities[] = {
	{"lsquo", "‘"},
	{"rsquo", "’"},
	{"sbquo", "‚"},
	{"ldquo", "“"},
	{"rdquo", "”"},
	{"bdquo", "„"},
	{"quot", "\""},
	{"apos", "'"},
	{"num", "#"},
	{"trade", "™"},
	{"copy", "©"},
	{"reg", "®"},
	{"numero", "№"},
	{"sup1", "¹"},
	{"sup2", "²"},
	{"sup3", "³"},
	{"amp", "&"},
	{"lt", "<"},
	{"gt", ">"},
	{"half", "½"},
	{"frac12", "½"},
	{"frac13", "⅓"},
	{"frac23", "⅔"},
	{"frac14", "¼"},
	{"frac34", "¾"},
	{"frac15", "⅕"},
	{"frac25", "⅖"},
	{"frac35", "⅗"},
	{"frac45", "⅘"},
	{"frac16", "⅙"},
	{"frac56", "⅚"},
	{"frac18", "⅛"},
	{"frac58", "⅝"},
	{"frac78", "⅞"},
	{"deg", "°"},
	{"permil", "‰"},
	{"dollar", "$"},
	{"euro", "€"},
	{"yen", "¥"},
	{"pound", "£"},
	{"cent", "¢"},
	{"hearts", "♥"},
	{"clubs", "♣"},
	{"diams", "♦"},
	{"spades", "♠"},
	{"pertenk", "‱"},
};

static const char *
translate_entity(char *entity_name)
{
	for (size_t i = 0; i < LENGTH(entities); ++i) {
		if (strcmp(entity_name, entities[i].name) == 0) {
			return entities[i].value;
		}
	}
	return NULL;
}

struct string *
expand_html_entities(char *buf, size_t buf_len)
{
	char *data = malloc(sizeof(char) * (buf_len + 1));
	if (data == NULL) {
		return NULL;
	}
	size_t data_len = 0;
	bool in_entity = false;
	char entity_name[MAX_ENTITY_NAME_LENGTH + 1];
	const char *entity_value;
	size_t entity_len;
	for (size_t i = 0; i < buf_len; ++i) {
		if (in_entity == true) {
			if (buf[i] == ';') {
				in_entity = false;
				entity_name[entity_len] = '\0';
				entity_value = translate_entity(entity_name);
				if (entity_value != NULL) {
					data[data_len] = '\0';
					strcat(data, entity_value);
					data_len += strlen(entity_value);
				}
			} else {
				if (entity_len == MAX_ENTITY_NAME_LENGTH) {
					in_entity = false;
					entity_name[entity_len] = '\0';
					data[data_len] = '\0';
					strcat(data, "&");
					strcat(data, entity_name);
					data_len += strlen(entity_name) + 1;
				} else {
					entity_name[entity_len++] = buf[i];
				}
			}
		} else {
			if (buf[i] == '&') {
				in_entity = true;
				entity_len = 0;
			} else {
				data[data_len++] = buf[i];
			}
		}
	}

	data[data_len] = '\0';

	if (in_entity == true) {
		entity_name[entity_len] = '\0';
		strcat(data, "&");
		strcat(data, entity_name);
		data_len += strlen(entity_name) + 1;
	}

	struct string *text_buf = malloc(sizeof(struct string));
	if (text_buf == NULL) {
		free(data);
		return NULL;
	}
	text_buf->ptr = data;
	text_buf->len = data_len;

	return text_buf;
}
