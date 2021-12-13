#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

#define INITIAL_TAG_BUFFER_SIZE 1000 // must be greater than 1
#define MAX_NESTED_LISTS_DEPTH 10

static void add_one_newline(void);
static void add_two_newlines(void);
static void add_three_newlines(void);
static void br_handler(void);
static void hr_handler(void);
static void li_start_handler(void);
static void ul_start_handler(void);
static void ul_end_handler(void);
static void ol_start_handler(void);
static void sup_start_handler(void);
static void q_start_handler(void);

enum html_position {
	HTML_NONE = 0,
	HTML_PRE = 1,
	HTML_STYLE = 2,
	HTML_SCRIPT = 4,
};

enum list_type {
	UNORDERED_LIST,
	ORDERED_LIST,
};

struct list_level {
	enum list_type type;
	int length;
};

struct tag_handler {
	const char *const name;
	const enum html_position pos;
	void (*const start_handler)(void);
	void (*const end_handler)(void);
};

static const struct tag_handler tag_handlers[] = {
	{"span",       HTML_NONE,   NULL,                NULL},
	{"code",       HTML_NONE,   NULL,                NULL}, // TODO
	{"a",          HTML_NONE,   NULL,                NULL}, // TODO
	{"b",          HTML_NONE,   NULL,                NULL}, // TODO
	{"i",          HTML_NONE,   NULL,                NULL}, // TODO
	{"em",         HTML_NONE,   NULL,                NULL}, // TODO
	{"mark",       HTML_NONE,   NULL,                NULL}, // TODO
	{"small",      HTML_NONE,   NULL,                NULL}, // TODO
	{"time",       HTML_NONE,   NULL,                NULL}, // TODO
	{"strong",     HTML_NONE,   NULL,                NULL}, // TODO
	{"p",          HTML_NONE,   add_two_newlines,    add_two_newlines},
	{"details",    HTML_NONE,   add_two_newlines,    add_two_newlines},
	{"blockquote", HTML_NONE,   add_two_newlines,    add_two_newlines},
	{"pre",        HTML_PRE,    add_two_newlines,    add_two_newlines},
	{"div",        HTML_NONE,   add_one_newline,     add_one_newline},
	{"section",    HTML_NONE,   add_one_newline,     add_one_newline},
	{"footer",     HTML_NONE,   add_one_newline,     add_one_newline},
	{"summary",    HTML_NONE,   add_one_newline,     add_one_newline},
	{"li",         HTML_NONE,   li_start_handler,    add_one_newline},
	{"ul",         HTML_NONE,   ul_start_handler,    ul_end_handler},
	{"ol",         HTML_NONE,   ol_start_handler,    ul_end_handler}, // it is not a typo
	{"br",         HTML_NONE,   br_handler,          NULL},
	{"hr",         HTML_NONE,   hr_handler,          NULL},
	{"h1",         HTML_NONE,   add_three_newlines,  add_two_newlines},
	{"h2",         HTML_NONE,   add_three_newlines,  add_two_newlines},
	{"h3",         HTML_NONE,   add_three_newlines,  add_two_newlines},
	{"h4",         HTML_NONE,   add_three_newlines,  add_two_newlines},
	{"h5",         HTML_NONE,   add_three_newlines,  add_two_newlines},
	{"h6",         HTML_NONE,   add_three_newlines,  add_two_newlines},
	{"sup",        HTML_NONE,   sup_start_handler,   NULL},
	{"q",          HTML_NONE,   q_start_handler,     q_start_handler},
	{"style",      HTML_STYLE,  NULL,                NULL},
	{"script",     HTML_SCRIPT, NULL,                NULL},
};

// Well...
// Functions down there will be called hella lot of times per second!
// Do we really need to ask our poor stack every single time to allocate?
// This is real question, but for now leave everything in the lifetime of
// the program because this might be a ?good? ?performance? ?boost?
static char *text;
static size_t text_len;
static char *tag_name;
static size_t tag_name_len;
static bool is_end_tag;
static uint8_t list_depth;
static struct list_level list_levels[MAX_NESTED_LISTS_DEPTH];
static enum html_position position;
static char *tag;
static size_t tag_len;
static size_t tag_lim;
static char temp;
static char *temp_ptr;

static void
add_one_newline(void)
{
	if (text_len >= 1 && text[text_len - 1] == '\n') {
		// nothing
	} else {
		text[text_len++] = '\n';
	}
}

static void
add_two_newlines(void)
{
	if (text_len >= 1 && text[text_len - 1] == '\n') {
		if (text_len >= 2 && text[text_len - 2] == '\n') {
			// nothing
		} else {
			text[text_len++] = '\n';
		}
	} else {
		text[text_len++] = '\n';
		text[text_len++] = '\n';
	}
}

static void
add_three_newlines(void)
{
	if (text_len >= 1 && text[text_len - 1] == '\n') {
		if (text_len >= 2 && text[text_len - 2] == '\n') {
			if (text_len >= 3 && text[text_len - 3] == '\n') {
				// nothing
			} else {
				text[text_len++] = '\n';
			}
		} else {
			text[text_len++] = '\n';
			text[text_len++] = '\n';
		}
	} else {
		text[text_len++] = '\n';
		text[text_len++] = '\n';
		text[text_len++] = '\n';
	}
}

static void
br_handler(void)
{
	text[text_len++] = '\n';
}

static void
hr_handler(void)
{
	add_one_newline();
	for (int i = 0; i < list_menu_width; ++i) {
		text[text_len++] = '-';
	}
	text[text_len++] = '\n';
}

static void
li_start_handler(void)
{
	add_one_newline();
	for (uint8_t i = 0; i < list_depth; ++i) {
		text[text_len++] = ' ';
		text[text_len++] = ' ';
		text[text_len++] = ' ';
	}
	if (list_levels[list_depth - 1].type == UNORDERED_LIST) {
		text[text_len++] = '*';
		text[text_len++] = ' ';
		text[text_len++] = ' ';
	} else {
		++(list_levels[list_depth - 1].length);
		// 14 because 11 (max length of int) + 2 (dot and space) + 1 (terminator)
		char number_str[14];
		snprintf(number_str, 14, "%d. ", list_levels[list_depth - 1].length);
		text[text_len] = '\0';
		strcat(text, number_str);
		text_len += strlen(number_str);
	}
}

static void
ul_start_handler(void)
{
	if (list_depth == MAX_NESTED_LISTS_DEPTH) {
		return;
	}
	add_two_newlines();
	++list_depth;
	list_levels[list_depth - 1].type = UNORDERED_LIST;
}

static void
ul_end_handler(void)
{
	add_two_newlines();
	if (list_depth > 0) {
		--list_depth;
	}
}

static void
ol_start_handler(void)
{
	if (list_depth == MAX_NESTED_LISTS_DEPTH) {
		return;
	}
	add_two_newlines();
	++list_depth;
	list_levels[list_depth - 1].type = ORDERED_LIST;
	list_levels[list_depth - 1].length = 0;
}

static void
sup_start_handler(void)
{
	text[text_len++] = '^';
}

static void
q_start_handler(void)
{
	text[text_len++] = '"';
}

// Append some characters according to the current position in the HTML document.
static inline void
format_text(char *tag, size_t tag_len)
{
	if (tag[0] == '/') {
		tag_name = tag + 1;
		is_end_tag = true;
	} else {
		tag_name = tag;
		is_end_tag = false;
	}
	tag_name_len = 0;

	for (size_t i = 0; i < tag_len; ++i) {
		if ((tag[i] == ' ') || (tag[i] == '\n') || (tag[i] == '\t')) {
			break;
		}
		++tag_name_len;
	}

	temp = tag_name[tag_name_len];
	tag_name[tag_name_len] = '\0';

	for (size_t i = 0; i < LENGTH(tag_handlers); ++i) {
		if (strcmp(tag_name, tag_handlers[i].name) == 0) {
			tag_name[tag_name_len] = temp;
			if (is_end_tag == true) {
				if (tag_handlers[i].pos != HTML_NONE) {
					position &= ~tag_handlers[i].pos;
				}
				if (tag_handlers[i].end_handler != NULL) {
					tag_handlers[i].end_handler();
				}
			} else {
				if (tag_handlers[i].pos != HTML_NONE) {
					position |= tag_handlers[i].pos;
				}
				if (tag_handlers[i].start_handler != NULL) {
					tag_handlers[i].start_handler();
				}
			}
			return;
		}
	}

	tag_name[tag_name_len] = temp;

	text[text_len++] = '<';
	text[text_len] = '\0';
	strcat(text, tag);
	text_len += tag_len;
	text[text_len++] = '>';
}

struct string *
plainify_html(const char *str, size_t str_len)
{
	text = malloc(sizeof(char) * (5 * str_len + 1));
	if (text == NULL) {
		return NULL;
	}
	tag = malloc(sizeof(char) * INITIAL_TAG_BUFFER_SIZE);
	if (tag == NULL) {
		free(text);
		return NULL;
	}
	text_len = 0;
	tag_lim = INITIAL_TAG_BUFFER_SIZE - 1;

	list_depth = 0;
	position = HTML_NONE;

	bool in_tag = false; // shows if str[i] character belongs to html tag
	bool error = false;  // shows if error occurred in loop

	for (size_t i = 0; i < str_len; ++i) {
		if (in_tag == false) {
			if (str[i] == '<') {
				in_tag = true;
				tag_len = 0;
			} else if ((position & (HTML_STYLE|HTML_SCRIPT)) != 0) {
				// Do not write characters of style and script elements.
				continue;
			} else if (((position & HTML_PRE) == 0) && ((str[i] == ' ') || (str[i] == '\n') || (str[i] == '\t'))) {
				if ((text_len != 0) && (text[text_len - 1] != ' ') && (text[text_len - 1] != '\n') && (text[text_len - 1] != '\t')) {
					text[text_len++] = ' ';
				}
			} else {
				text[text_len++] = str[i];
			}
		} else {
			if (str[i] == '>') {
				in_tag = false;
				tag[tag_len] = '\0';
				format_text(tag, tag_len);
			} else {
				if (tag_len == tag_lim) {
					tag_lim *= 2;
					temp_ptr = realloc(tag, sizeof(char) * (tag_lim + 1));
					if (temp_ptr == NULL) {
						error = true;
						break;
					}
					tag = temp_ptr;
				}
				tag[tag_len++] = str[i];
			}
		}
	}

	free(tag);

	if (error == true) {
		free(text);
		return NULL;
	}

	text[text_len] = '\0';

	struct string *expanded_text = expand_html_entities(text, text_len);
	free(text);
	if (expanded_text == NULL) {
		FAIL("Failed to expand HTML entities of contents!");
		return NULL;
	}

	strip_whitespace_from_edges(expanded_text->ptr, &(expanded_text->len));

	return expanded_text;
}
