#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

#define INITIAL_TAG_BUFFER_SIZE 1000 // must be greater than 1
#define MAX_NESTED_LISTS_DEPTH 10

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

// Well...
// Functions down there will be called hella lot of times per second!
// Do we really need to ask our poor stack every single time to allocate?
// This is real question, but for now leave these in the lifetime of
// the program because this might be a ?good? ?performance? ?boost?
static char *text;
static size_t text_len;
static uint8_t list_depth;
static struct list_level list_levels[MAX_NESTED_LISTS_DEPTH];
static enum html_position position;

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

static inline void
br_handler(void)
{
	text[text_len++] = '\n';
}

static inline void
hr_handler(void)
{
	add_one_newline();
	for (int i = 0; i < list_menu_width; ++i) {
		text[text_len++] = '-';
	}
	text[text_len++] = '\n';
}

static inline void
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

static inline void
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

static inline void
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

static inline void
sup_start_handler(void)
{
	text[text_len++] = '^';
}

static inline void
q_start_handler(void)
{
	text[text_len++] = '"';
}

static inline void
pre_start_handler(void)
{
	position |= HTML_PRE;
	add_two_newlines();
}

static inline void
pre_end_handler(void)
{
	position &= ~HTML_PRE;
	add_two_newlines();
}

static inline void
script_start_handler(void)
{
	position |= HTML_SCRIPT;
}

static inline void
script_end_handler(void)
{
	position &= ~HTML_SCRIPT;
}

static inline void
style_start_handler(void)
{
	position |= HTML_STYLE;
}

static inline void
style_end_handler(void)
{
	position &= ~HTML_STYLE;
}

static inline bool
start_handler(char *tag)
{
	     if (strcmp(tag, "span")       == 0) {                         return true; }
	else if (strcmp(tag, "p")          == 0) { add_two_newlines();     return true; }
	else if (strcmp(tag, "details")    == 0) { add_two_newlines();     return true; }
	else if (strcmp(tag, "br")         == 0) { br_handler();           return true; }
	else if (strcmp(tag, "li")         == 0) { li_start_handler();     return true; }
	else if (strcmp(tag, "ul")         == 0) { ul_start_handler();     return true; }
	else if (strcmp(tag, "ol")         == 0) { ol_start_handler();     return true; }
	else if (strcmp(tag, "h1")         == 0) { add_three_newlines();   return true; }
	else if (strcmp(tag, "h2")         == 0) { add_three_newlines();   return true; }
	else if (strcmp(tag, "h3")         == 0) { add_three_newlines();   return true; }
	else if (strcmp(tag, "h4")         == 0) { add_three_newlines();   return true; }
	else if (strcmp(tag, "h5")         == 0) { add_three_newlines();   return true; }
	else if (strcmp(tag, "h6")         == 0) { add_three_newlines();   return true; }
	else if (strcmp(tag, "div")        == 0) { add_one_newline();      return true; }
	else if (strcmp(tag, "section")    == 0) { add_one_newline();      return true; }
	else if (strcmp(tag, "footer")     == 0) { add_one_newline();      return true; }
	else if (strcmp(tag, "summary")    == 0) { add_one_newline();      return true; }
	else if (strcmp(tag, "sup")        == 0) { sup_start_handler();    return true; }
	else if (strcmp(tag, "hr")         == 0) { hr_handler();           return true; }
	else if (strcmp(tag, "blockquote") == 0) { add_two_newlines();     return true; }
	else if (strcmp(tag, "q")          == 0) { q_start_handler();      return true; }
	else if (strcmp(tag, "code")       == 0) { /* TODO */              return true; }
	else if (strcmp(tag, "a")          == 0) { /* TODO */              return true; }
	else if (strcmp(tag, "b")          == 0) { /* TODO */              return true; }
	else if (strcmp(tag, "i")          == 0) { /* TODO */              return true; }
	else if (strcmp(tag, "em")         == 0) { /* TODO */              return true; }
	else if (strcmp(tag, "mark")       == 0) { /* TODO */              return true; }
	else if (strcmp(tag, "small")      == 0) { /* TODO */              return true; }
	else if (strcmp(tag, "time")       == 0) { /* TODO */              return true; }
	else if (strcmp(tag, "strong")     == 0) { /* TODO */              return true; }
	else if (strcmp(tag, "pre")        == 0) { pre_start_handler();    return true; }
	else if (strcmp(tag, "script")     == 0) { script_start_handler(); return true; }
	else if (strcmp(tag, "style")      == 0) { style_start_handler();  return true; }

	return false;
}

static inline bool
end_handler(char *tag)
{
	     if (strcmp(tag, "span")       == 0) {                       return true; }
	else if (strcmp(tag, "p")          == 0) { add_two_newlines();   return true; }
	else if (strcmp(tag, "details")    == 0) { add_two_newlines();   return true; }
	else if (strcmp(tag, "li")         == 0) { add_one_newline();    return true; }
	else if (strcmp(tag, "ul")         == 0) { ul_end_handler();     return true; }
	else if (strcmp(tag, "ol")         == 0) { ul_end_handler();     return true; }
	else if (strcmp(tag, "h1")         == 0) { add_two_newlines();   return true; }
	else if (strcmp(tag, "h2")         == 0) { add_two_newlines();   return true; }
	else if (strcmp(tag, "h3")         == 0) { add_two_newlines();   return true; }
	else if (strcmp(tag, "h4")         == 0) { add_two_newlines();   return true; }
	else if (strcmp(tag, "h5")         == 0) { add_two_newlines();   return true; }
	else if (strcmp(tag, "h6")         == 0) { add_two_newlines();   return true; }
	else if (strcmp(tag, "div")        == 0) { add_one_newline();    return true; }
	else if (strcmp(tag, "section")    == 0) { add_one_newline();    return true; }
	else if (strcmp(tag, "footer")     == 0) { add_one_newline();    return true; }
	else if (strcmp(tag, "summary")    == 0) { add_one_newline();    return true; }
	else if (strcmp(tag, "blockquote") == 0) { add_two_newlines();   return true; }
	else if (strcmp(tag, "q")          == 0) { q_start_handler();    return true; }
	else if (strcmp(tag, "code")       == 0) { /* TODO */            return true; }
	else if (strcmp(tag, "a")          == 0) { /* TODO */            return true; }
	else if (strcmp(tag, "b")          == 0) { /* TODO */            return true; }
	else if (strcmp(tag, "i")          == 0) { /* TODO */            return true; }
	else if (strcmp(tag, "em")         == 0) { /* TODO */            return true; }
	else if (strcmp(tag, "mark")       == 0) { /* TODO */            return true; }
	else if (strcmp(tag, "small")      == 0) { /* TODO */            return true; }
	else if (strcmp(tag, "time")       == 0) { /* TODO */            return true; }
	else if (strcmp(tag, "strong")     == 0) { /* TODO */            return true; }
	else if (strcmp(tag, "pre")        == 0) { pre_end_handler();    return true; }
	else if (strcmp(tag, "script")     == 0) { script_end_handler(); return true; }
	else if (strcmp(tag, "style")      == 0) { style_end_handler();  return true; }

	return false;
}

// Append some characters according to the current position in the HTML document.
static inline int
format_text(char *tag, size_t tag_len)
{
	char *tag_name;
	bool is_end_tag;

	if (tag[0] == '/') {
		tag_name = tag + 1;
		is_end_tag = true;
	} else {
		tag_name = tag;
		is_end_tag = false;
	}

	size_t tag_name_len = 0;
	for (size_t i = 0; i < tag_len; ++i) {
		if ((tag[i] == ' ') || (tag[i] == '\n') || (tag[i] == '\t')) {
			break;
		}
		++tag_name_len;
	}

	char temp = tag_name[tag_name_len];
	tag_name[tag_name_len] = '\0';

	if (is_end_tag == true) {
		if (end_handler(tag_name) == true) {
			return 0;
		}
	} else {
		if (start_handler(tag_name) == true) {
			return 0;
		}
	}

	tag_name[tag_name_len] = temp;

	text[text_len++] = '<';
	text[text_len] = '\0';
	strcat(text, tag);
	text_len += tag_len;
	text[text_len++] = '>';

	return 0;
}

struct string *
plainify_html(const char *str, size_t str_len)
{
	text = malloc(sizeof(char) * (5 * str_len + 1));
	if (text == NULL) {
		return NULL;
	}
	char *tag = malloc(sizeof(char) * INITIAL_TAG_BUFFER_SIZE);
	if (tag == NULL) {
		free(text);
		return NULL;
	}
	text_len = 0;
	size_t tag_lim = INITIAL_TAG_BUFFER_SIZE - 1;

	list_depth = 0;
	position = HTML_NONE;

	size_t tag_len;
	bool in_tag = false; // shows if str[i] character belongs to html tag
	bool error = false;  // shows if error occurred in loop
	char *temp_ptr;

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
