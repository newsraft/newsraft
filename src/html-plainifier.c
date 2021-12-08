#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

static void br_handler(void);
static void hr_handler(void);
static void block_start_handler(void);
static void block_end_handler(void);
static void heading_start_handler(void);
static void heading_end_handler(void);
static void li_start_handler(void);
static void ul_start_handler(void);
static void ul_end_handler(void);
static void ol_start_handler(void);
static void ol_end_handler(void);
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

struct tag_handler {
	const char *const name;
	const enum html_position pos;
	void (*start_handler)(void);
	void (*end_handler)(void);
};

struct tag_handler tag_handlers[] = {
	{"a",          HTML_NONE,   NULL,                  NULL}, // TODO
	{"b",          HTML_NONE,   NULL,                  NULL}, // TODO
	{"i",          HTML_NONE,   NULL,                  NULL}, // TODO
	{"em",         HTML_NONE,   NULL,                  NULL}, // TODO
	{"mark",       HTML_NONE,   NULL,                  NULL}, // TODO
	{"small",      HTML_NONE,   NULL,                  NULL}, // TODO
	{"time",       HTML_NONE,   NULL,                  NULL}, // TODO
	{"strong",     HTML_NONE,   NULL,                  NULL}, // TODO
	{"p",          HTML_NONE,   heading_end_handler,   heading_end_handler},
	{"details",    HTML_NONE,   heading_end_handler,   heading_end_handler},
	{"blockquote", HTML_NONE,   heading_end_handler,   heading_end_handler},
	{"div",        HTML_NONE,   block_start_handler,   block_end_handler},
	{"summary",    HTML_NONE,   block_start_handler,   block_end_handler},
	{"span",       HTML_NONE,   NULL,                  NULL},
	{"code",       HTML_NONE,   NULL,                  NULL},
	{"pre",        HTML_PRE,    heading_end_handler,   heading_end_handler},
	{"li",         HTML_NONE,   li_start_handler,      block_end_handler},
	{"ul",         HTML_NONE,   ul_start_handler,      ul_end_handler},
	{"ol",         HTML_NONE,   ol_start_handler,      ol_end_handler},
	{"br",         HTML_NONE,   br_handler,            NULL},
	{"hr",         HTML_NONE,   hr_handler,            NULL},
	{"h1",         HTML_NONE,   heading_start_handler, heading_end_handler},
	{"h2",         HTML_NONE,   heading_start_handler, heading_end_handler},
	{"h3",         HTML_NONE,   heading_start_handler, heading_end_handler},
	{"h4",         HTML_NONE,   heading_start_handler, heading_end_handler},
	{"h5",         HTML_NONE,   heading_start_handler, heading_end_handler},
	{"h6",         HTML_NONE,   heading_start_handler, heading_end_handler},
	{"sup",        HTML_NONE,   sup_start_handler,     NULL},
	{"q",          HTML_NONE,   q_start_handler,       q_start_handler},
	{"section",    HTML_NONE,   heading_start_handler, heading_end_handler},
	{"footer",     HTML_NONE,   heading_start_handler, heading_end_handler},
	{"style",      HTML_STYLE,  NULL,                  NULL},
	{"script",     HTML_SCRIPT, NULL,                  NULL},

};

// Well...
// Functions down there will be called hella lot of times per second!
// Do we really need to ask our poor stack every single time to allocate?
// This is real question, but for now leave everything in
// the lifetime of the program because this might be a ?good? ?performance? ?boost?
static enum list_type current_list_type = UNORDERED_LIST;
static char *text;
static size_t text_len;
static char *tag_name;
static size_t tag_name_len;
static bool is_end_tag;
static uint8_t list_depth;
static uint8_t lists_length[100]; // make dynamic TODO
static enum html_position position;
static char temp;
static char tag[1000]; // make dynamic TODO
static size_t tag_len;

static void
br_handler(void)
{
	text[text_len++] = '\n';
}

static void
block_start_handler(void)
{
	if (text_len >= 1 && text[text_len - 1] == '\n') {
		// nothing
	} else {
		text[text_len++] = '\n';
	}
}

static void
block_end_handler(void)
{
	if (text_len >= 1 && text[text_len - 1] == '\n') {
		// nothing
	} else {
		text[text_len++] = '\n';
	}
}

static void
hr_handler(void)
{
	block_start_handler();
	for (int i = 0; i < list_menu_width; ++i) {
		text[text_len++] = '-';
	}
	text[text_len++] = '\n';
}

static void
heading_start_handler(void)
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
heading_end_handler(void)
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
li_start_handler(void)
{
	block_start_handler();
	for (uint8_t i = 0; i < list_depth; ++i) {
		text[text_len++] = ' ';
		text[text_len++] = ' ';
		text[text_len++] = ' ';
	}
	if (current_list_type == UNORDERED_LIST) {
		text[text_len++] = '*';
		text[text_len++] = ' ';
		text[text_len++] = ' ';
	} else {
		++lists_length[list_depth - 1];
		char *number_str = malloc(sizeof(char) * 10);
		if (number_str != NULL) {
			snprintf(number_str, 10, "%d. ", lists_length[list_depth - 1]);
			text[text_len] = '\0';
			strcat(text, number_str);
			text_len += strlen(number_str);
			free(number_str);
		}
	}
}

static void
ul_start_handler(void)
{
	heading_end_handler();
	current_list_type = UNORDERED_LIST;
	++list_depth;
}

static void
ul_end_handler(void)
{
	heading_end_handler();
	--list_depth;
}

static void
ol_start_handler(void)
{
	heading_end_handler();
	current_list_type = ORDERED_LIST;
	++list_depth;
	lists_length[list_depth - 1] = 0;
}

static void
ol_end_handler(void)
{
	heading_end_handler();
	current_list_type = UNORDERED_LIST;
	--list_depth;
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
	text[text_len] = '\0';
}

struct string *
plainify_html(const char *str, size_t str_len)
{
	text = malloc(sizeof(char) * (5 * str_len + 1));
	if (text == NULL) {
		return NULL;
	}
	text_len = 0;
	position = HTML_NONE;

	list_depth = 0;
	bool   in_tag = false; // shows if str[i] character belongs to html tag
	bool   error = false;  // shows if error occurred in loop

	for (size_t i = 0; i < str_len; ++i) {
		if (in_tag == false) {
			if (str[i] == '<') {
				in_tag = true;
				tag_len = 0;
			} else if (((position & HTML_STYLE) != 0) || ((position & HTML_SCRIPT) != 0)) {
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
				tag[tag_len++] = str[i];
			}
		}
	}

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
