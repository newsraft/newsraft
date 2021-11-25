#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "feedeater.h"

// This has to be the length of the longest entity name in entities array.
#define MAX_ENTITY_NAME_LENGTH 13

struct entity_entry {
	const char *name;
	// store numbers as strings to compare them without conversion
	const char *number;
	const char *hex_number;
	const char *value;
};

// WARNING!
// Before you add here an entity whose value is longer than name,
// you have to make the data buffer dynamic to avoid overflow!
// Also don't forget to update MAX_ENTITY_NAME_LENGTH every time
// you add another entity with the longest name.
static const struct entity_entry entities[] = {
	{"nbsp",          "160",   "A0",    " "},
	{"lsquo",         "8216",  "2018",  "‘"},
	{"rsquo",         "8217",  "2019",  "’"},
	{"rsquor",        "8217",  "2019",  "’"},
	{"sbquo",         "8218",  "201A",  "‚"},
	{"lsquor",        "8218",  "201A",  "‚"},
	{"ldquo",         "8220",  "201C",  "“"},
	{"rdquo",         "8221",  "201D",  "”"},
	{"rdquor",        "8221",  "201D",  "”"},
	{"bdquo",         "8222",  "201E",  "„"},
	{"ldquor",        "8222",  "201E",  "„"},
	{"laquo",         "171",   "AB",    "«"},
	{"raquo",         "187",   "BB",    "»"},
	{"dash",          "8208",  "2010",  "-"},
	{"hyphen",        "8208",  "2010",  "-"},
	{"ndash",         "8211",  "2013",  "–"},
	{"mdash",         "8212",  "2014",  "—"},
	{"horbar",        "8213",  "2015",  "―"},
	{"quot",          "34",    "22",    "\""},
	{"apos",          "39",    "27",    "'"},
	{"num",           "35",    "23",    "#"},
	{"colon",         "58",    "3A",    ":"},
	{"semi",          "59",    "3B",    ";"},
	{"lsaquo",        "8249",  "2039",  "‹"},
	{"rsaquo",        "8250",  "203A",  "›"},
	{"hellip",        "8230",  "2026",  "…"},
	{"mldr",          "8230",  "2026",  "…"},
	{"larrhk",        "8617",  "21A9",  "↩"},
	{"hookleftarrow", "8617",  "21A9",  "↩"},
	{"trade",         "8482",  "2122",  "™"},
	{"copy",          "169",   "A9",    "©"},
	{"reg",           "174",   "AE",    "®"},
	{"numero",        "8470",  "2116",  "№"},
	{"sect",          "167",   "A7",    "§"},
	{"sup1",          "185",   "B9",    "¹"},
	{"sup2",          "178",   "B2",    "²"},
	{"sup3",          "179",   "B3",    "³"},
	{"amp",           "38",    "26",    "&"},
	{"lt",            "60",    "3C",    "<"},
	{"gt",            "62",    "3E",    ">"},
	{"rightarrow",    "8594",  "2192",  "→"},
	{"RightArrow",    "8594",  "2192",  "→"},
	{"rarr",          "8594",  "2192",  "→"},
	{"Rightarrow",    "8658",  "21D2",  "⇒"},
	{"rArr",          "8658",  "21D2",  "⇒"},
	{"Rarr",          "8608",  "21A0",  "↠"},
	{"rdsh",          "8627",  "21B3",  "↳"},
	{"rdca",          "10551", "2937",  "⤷"},
	{NULL,            "65038", "FE0E",  ""},
	{NULL,            "65039", "FE0F",  ""},
	{"half",          "189",   "BD",    "½"},
	{"frac12",        "189",   "BD",    "½"},
	{"frac13",        "8531",  "2153",  "⅓"},
	{"frac23",        "8532",  "2154",  "⅔"},
	{"frac14",        "188",   "BC",    "¼"},
	{"frac34",        "190",   "BE",    "¾"},
	{"frac15",        "8533",  "2155",  "⅕"},
	{"frac25",        "8534",  "2156",  "⅖"},
	{"frac35",        "8535",  "2157",  "⅗"},
	{"frac45",        "8536",  "2158",  "⅘"},
	{"frac16",        "8537",  "2159",  "⅙"},
	{"frac56",        "8538",  "215A",  "⅚"},
	{"frac18",        "8539",  "215B",  "⅛"},
	{"frac38",        "8540",  "215C",  "⅜"},
	{"frac58",        "8541",  "215D",  "⅝"},
	{"frac78",        "8542",  "215E",  "⅞"},
	{"check",         "10003", "2713",  "✓"},
	{"checkmark",     "10003", "2713",  "✓"},
	{"deg",           "176",   "B0",    "°"},
	{"divide",        "247",   "F7",    "÷"},
	{"div",           "247",   "F7",    "÷"},
	{"dagger",        "8224",  "2020",  "†"},
	{"permil",        "8240",  "2030",  "‰"},
	{"dollar",        "36",    "24",    "$"},
	{"euro",          "8364",  "20AC",  "€"},
	{"pound",         "163",   "A3",    "£"},
	{"cent",          "162",   "A2",    "¢"},
	{"yen",           "165",   "A5",    "¥"},
	{"hearts",        "9829",  "2665",  "♥"},
	{"heartsuit",     "9829",  "2665",  "♥"},
	{"diams",         "9830",  "2666",  "♦"},
	{"diamondsuit",   "9830",  "2666",  "♦"},
	{"clubs",         "9827",  "2663",  "♣"},
	{"clubsuit",      "9827",  "2663",  "♣"},
	{"spades",        "9824",  "2660",  "♠"},
	{"spadesuit",     "9824",  "2660",  "♠"},
	{"pertenk",       "8241",  "2031",  "‱"},
};

static size_t
count_insignificant_zeros(char *entity)
{
	size_t insignificant_zeros_count = 0;
	for (size_t i = 0; entity[i] != '\0'; ++i) {
		if (entity[i] == '0') {
			++insignificant_zeros_count;
		} else {
			break;
		}
	}
	return insignificant_zeros_count;
}

static const char *
translate_entity(char *entity)
{
	if (entity[0] == '#') { // entity is of number type
		// Shift by 1 character because we already know that 1st character is a '#'.
		char *entity_number = entity + 1;
		if (entity_number[0] == 'x') { // entity is of hexadecimal number type
			// Shift by 1 more character because we know that 1st character of entity_number is 'x'.
			entity_number += 1;
			entity_number += count_insignificant_zeros(entity_number);
			// Convert all hex digits of entity to upper case.
			for (size_t i = 0; entity_number[i] != '\0'; ++i) {
				entity_number[i] = toupper(entity_number[i]);
			}
			for (size_t i = 0; i < LENGTH(entities); ++i) {
				if (strcmp(entity_number, entities[i].hex_number) == 0) {
					return entities[i].value;
				}
			}
		} else { // entity is of decimal number type
			entity_number += count_insignificant_zeros(entity_number);
			for (size_t i = 0; i < LENGTH(entities); ++i) {
				if (strcmp(entity_number, entities[i].number) == 0) {
					return entities[i].value;
				}
			}
		}
	} else { // entity is of word type
		for (size_t i = 0; i < LENGTH(entities); ++i) {
			if ((entities[i].name != NULL) && (strcmp(entity, entities[i].name) == 0)) {
				return entities[i].value;
			}
		}
	}
	debug_write(DBG_WARN, "Met an unknown HTML entity: %s\n", entity);
	return NULL;
}

struct string *
expand_html_entities(char *buf, size_t buf_len)
{
	char *data = malloc(sizeof(char) * (buf_len + 1));
	if (data == NULL) {
		debug_write(DBG_FAIL, "Could not allocate enough memory for expanding HTML entities of item contents!\n");
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
				} else {
					data[data_len++] = '&';
					data[data_len] = '\0';
					strcat(data, entity_name);
					data_len += entity_len;
					data[data_len++] = ';';
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
