#include <stdlib.h>
#include <string.h>
#include "render_data.h"

enum html_position {
	HTML_NONE = 0,
	HTML_PRE = 1,
};

struct html_element_handler {
	const GumboTag tag_id;
	void (*start_handler)(struct wstring *, struct line *, enum html_position *);
	void (*end_handler)(struct wstring *, struct line *, enum html_position *);
};

#define MAX_NESTED_LISTS_DEPTH 10
#define SPACES_PER_INDENTATION_LEVEL 4
#define SPACES_PER_BLOCKQUOTE_LEVEL 4
#define SPACES_PER_FIGURE_LEVEL 4
#define SPACES_PER_DESCRIPTION_LEVEL 4

enum list_type {
	UNORDERED_LIST,
	ORDERED_LIST,
};

struct list_level {
	enum list_type type;
	uint16_t length;
};

static uint8_t list_depth;
static struct list_level list_levels[MAX_NESTED_LISTS_DEPTH];

static inline void
provide_newlines(struct wstring *text, struct line *line, int8_t count)
{
	int8_t how_many_newlines_already_present = 0;
	if (line->len == 0) {
		for (int8_t i = 1; i <= count; ++i) {
			if ((text->len >= (size_t)i) && (text->ptr[text->len - i] == L'\n')) {
				++how_many_newlines_already_present;
			} else {
				break;
			}
		}
	}
	for (int8_t i = 0; i < count - how_many_newlines_already_present; ++i) {
		line_char(line, L'\n', text);
	}
}

static void
provide_one_newline(struct wstring *text, struct line *line, enum html_position *pos)
{
	(void)pos;
	provide_newlines(text, line, 1);
}

static void
provide_two_newlines(struct wstring *text, struct line *line, enum html_position *pos)
{
	(void)pos;
	provide_newlines(text, line, 2);
}

static void
provide_three_newlines(struct wstring *text, struct line *line, enum html_position *pos)
{
	(void)pos;
	provide_newlines(text, line, 3);
}

static void
br_handler(struct wstring *text, struct line *line, enum html_position *pos)
{
	(void)pos;
	line_char(line, L'\n', text);
}

static void
hr_handler(struct wstring *text, struct line *line, enum html_position *pos)
{
	size_t temp_indent = line->indent;
	line->indent = 0;
	provide_one_newline(text, line, pos);
	for (size_t i = 1; i < line->lim; ++i) {
		line_char(line, L'─', text);
	}
	line_char(line, L'\n', text);
	line->indent = temp_indent;
}

static void
li_handler(struct wstring *text, struct line *line, enum html_position *pos)
{
	provide_one_newline(text, line, pos);
	line->indent = list_depth * SPACES_PER_INDENTATION_LEVEL;
	if (list_levels[list_depth - 1].type == UNORDERED_LIST) {
		line_string(line, L"*  ", text);
		line->indent += 3;
	} else {
		++(list_levels[list_depth - 1].length);
		// 9 = 5 (for longest uint16_t) + 2 (for dot and space) + 1 (for terminator) + 1 (for luck lol)
		wchar_t number_str[9];
		size_t number_str_len = swprintf(number_str, 9, L"%d. ", list_levels[list_depth - 1].length);
		line_string(line, number_str, text);
		line->indent += number_str_len;
	}
}

static void
ul_start_handler(struct wstring *text, struct line *line, enum html_position *pos)
{
	if (list_depth == MAX_NESTED_LISTS_DEPTH) {
		return;
	}
	if (list_depth == 0) {
		provide_two_newlines(text, line, pos);
	} else {
		provide_one_newline(text, line, pos);
	}
	++list_depth;
	list_levels[list_depth - 1].type = UNORDERED_LIST;
}

static void
ul_end_handler(struct wstring *text, struct line *line, enum html_position *pos)
{
	if (list_depth > 0) {
		--list_depth;
		line->indent = list_depth * SPACES_PER_INDENTATION_LEVEL;
	}
	if (list_depth == 0) {
		provide_two_newlines(text, line, pos);
	} else {
		provide_one_newline(text, line, pos);
	}
}

static void
ol_start_handler(struct wstring *text, struct line *line, enum html_position *pos)
{
	if (list_depth == MAX_NESTED_LISTS_DEPTH) {
		return;
	}
	if (list_depth == 0) {
		provide_two_newlines(text, line, pos);
	} else {
		provide_one_newline(text, line, pos);
	}
	++list_depth;
	list_levels[list_depth - 1].type = ORDERED_LIST;
	list_levels[list_depth - 1].length = 0;
}

static void
blockquote_start_handler(struct wstring *text, struct line *line, enum html_position *pos)
{
	provide_two_newlines(text, line, pos);
	line->indent += SPACES_PER_BLOCKQUOTE_LEVEL;
}

static void
blockquote_end_handler(struct wstring *text, struct line *line, enum html_position *pos)
{
	provide_two_newlines(text, line, pos);
	if (line->indent >= SPACES_PER_BLOCKQUOTE_LEVEL) {
		line->indent -= SPACES_PER_BLOCKQUOTE_LEVEL;
	}
}

static void
figure_start_handler(struct wstring *text, struct line *line, enum html_position *pos)
{
	provide_two_newlines(text, line, pos);
	line->indent += SPACES_PER_FIGURE_LEVEL;
}

static void
figure_end_handler(struct wstring *text, struct line *line, enum html_position *pos)
{
	provide_two_newlines(text, line, pos);
	if (line->indent >= SPACES_PER_FIGURE_LEVEL) {
		line->indent -= SPACES_PER_FIGURE_LEVEL;
	}
}

static void
dd_start_handler(struct wstring *text, struct line *line, enum html_position *pos)
{
	provide_one_newline(text, line, pos);
	line->indent += SPACES_PER_DESCRIPTION_LEVEL;
}

static void
dd_end_handler(struct wstring *text, struct line *line, enum html_position *pos)
{
	provide_one_newline(text, line, pos);
	if (line->indent >= SPACES_PER_DESCRIPTION_LEVEL) {
		line->indent -= SPACES_PER_DESCRIPTION_LEVEL;
	}
}

static void
pre_start_handler(struct wstring *text, struct line *line, enum html_position *pos)
{
	provide_two_newlines(text, line, pos);
	*pos |= HTML_PRE;
}

static void
pre_end_handler(struct wstring *text, struct line *line, enum html_position *pos)
{
	provide_two_newlines(text, line, pos);
	*pos &= ~HTML_PRE;
}

static const struct html_element_handler handlers[] = {
	{GUMBO_TAG_P,          &provide_two_newlines,     &provide_two_newlines},
	{GUMBO_TAG_DL,         &provide_two_newlines,     &provide_two_newlines},
	{GUMBO_TAG_BR,         &br_handler,               NULL},
	{GUMBO_TAG_LI,         &li_handler,               &provide_one_newline},
	{GUMBO_TAG_UL,         &ul_start_handler,         &ul_end_handler},
	{GUMBO_TAG_OL,         &ol_start_handler,         &ul_end_handler},
	{GUMBO_TAG_PRE,        &pre_start_handler,        &pre_end_handler},
	{GUMBO_TAG_H1,         &provide_three_newlines,   &provide_two_newlines},
	{GUMBO_TAG_H2,         &provide_three_newlines,   &provide_two_newlines},
	{GUMBO_TAG_H3,         &provide_three_newlines,   &provide_two_newlines},
	{GUMBO_TAG_H4,         &provide_three_newlines,   &provide_two_newlines},
	{GUMBO_TAG_H5,         &provide_three_newlines,   &provide_two_newlines},
	{GUMBO_TAG_H6,         &provide_three_newlines,   &provide_two_newlines},
	{GUMBO_TAG_HR,         &hr_handler,               NULL},
	{GUMBO_TAG_FIGURE,     &figure_start_handler,     &figure_end_handler},
	{GUMBO_TAG_BLOCKQUOTE, &blockquote_start_handler, &blockquote_end_handler},
	{GUMBO_TAG_DD,         &dd_start_handler,         &dd_end_handler},
	{GUMBO_TAG_DT,         &provide_one_newline,      &provide_one_newline},
	{GUMBO_TAG_DIV,        &provide_one_newline,      &provide_one_newline},
	{GUMBO_TAG_MAIN,       &provide_one_newline,      &provide_one_newline},
	{GUMBO_TAG_ARTICLE,    &provide_one_newline,      &provide_one_newline},
	{GUMBO_TAG_SUMMARY,    &provide_one_newline,      &provide_one_newline},
	{GUMBO_TAG_DETAILS,    &provide_two_newlines,     &provide_two_newlines},
	{GUMBO_TAG_ADDRESS,    &provide_one_newline,      &provide_one_newline},
	{GUMBO_TAG_FIGCAPTION, &provide_one_newline,      &provide_one_newline},
	{GUMBO_TAG_SECTION,    &provide_one_newline,      &provide_one_newline},
	{GUMBO_TAG_FOOTER,     &provide_one_newline,      &provide_one_newline},
	{GUMBO_TAG_OPTION,     &provide_one_newline,      &provide_one_newline},
	{GUMBO_TAG_FORM,       &provide_one_newline,      &provide_one_newline},
	{GUMBO_TAG_ASIDE,      &provide_one_newline,      &provide_one_newline},
	{GUMBO_TAG_NAV,        &provide_one_newline,      &provide_one_newline},
	{GUMBO_TAG_TABLE,      NULL,                      NULL},
	{GUMBO_TAG_UNKNOWN,    NULL,                      NULL},
};

static void
dump_html(GumboNode *node, struct wstring *text, struct line *line, enum html_position *pos)
{
	if (node->type == GUMBO_NODE_ELEMENT) {
		size_t i;
		for (i = 0; handlers[i].tag_id != GUMBO_TAG_UNKNOWN; ++i) {
			if (node->v.element.tag == handlers[i].tag_id) {
				break;
			}
		}
		if (handlers[i].tag_id == GUMBO_TAG_UNKNOWN) {
			struct string *tag = crtas(node->v.element.original_tag.data, node->v.element.original_tag.length);
			struct wstring *wtag;
			if (tag != NULL) {
				wtag = convert_string_to_wstring(tag);
				free_string(tag);
				if (wtag != NULL) {
					line_string(line, wtag->ptr, text);
					free_wstring(wtag);
				}
			}
			for (size_t j = 0; j < node->v.element.children.length; ++j) {
				dump_html(node->v.element.children.data[j], text, line, pos);
			}
			tag = crtas(node->v.element.original_end_tag.data, node->v.element.original_end_tag.length);
			if (tag != NULL) {
				wtag = convert_string_to_wstring(tag);
				free_string(tag);
				if (wtag != NULL) {
					line_string(line, wtag->ptr, text);
					free_wstring(wtag);
				}
			}
		} else if (handlers[i].tag_id == GUMBO_TAG_TABLE) {
			struct html_table *table = create_html_table();
			if (table != NULL) {
				for (size_t j = 0; j < node->v.element.children.length; ++j) {
					dump_html_table(node->v.element.children.data[j], table);
				}
				print_html_table_and_free_it(table, text, line);
			}
		} else {
			if (handlers[i].start_handler != NULL) {
				handlers[i].start_handler(text, line, pos);
			}
			for (size_t j = 0; j < node->v.element.children.length; ++j) {
				dump_html(node->v.element.children.data[j], text, line, pos);
			}
			if (handlers[i].end_handler != NULL) {
				handlers[i].end_handler(text, line, pos);
			}
		}
	} else if ((node->type == GUMBO_NODE_TEXT)
		|| (node->type == GUMBO_NODE_CDATA)
		|| (node->type == GUMBO_NODE_WHITESPACE))
	{
		struct string *str = crtas(node->v.text.text, strlen(node->v.text.text));
		if (str != NULL) {
			struct wstring *wstr = convert_string_to_wstring(str);
			free_string(str);
			if (wstr != NULL) {
				for (const wchar_t *i = wstr->ptr; *i != L'\0'; ++i) {
					if (!ISWIDEWHITESPACE(*i)) {
						line_char(line, *i, text);
					} else if ((*pos & HTML_PRE) != 0) {
						if (*i == '\n') {
							line_char(line, L'\n', text);
						} else if (*i == '\t') {
							line_string(line, L"    ", text);
						} else {
							line_char(line, L' ', text);
						}
					} else if (line->len > 0) {
						if (!ISWIDEWHITESPACE(line->ptr[line->len - 1])) {
							line_char(line, L' ', text);
						}
					} else if (text->len > 0) {
						if (!ISWIDEWHITESPACE(text->ptr[text->len - 1])) {
							line_char(line, L' ', text);
						}
					}
				}
				free_wstring(wstr);
			}
		}
	}
}

bool
render_text_html(const struct wstring *wstr, struct line *text_line, struct wstring *text, bool is_first_call)
{
	enum html_position html_pos = HTML_NONE;
	if (is_first_call == true) {
		list_depth = 0;
	}
	struct string *str = convert_wstring_to_string(wstr);
	if (str == NULL) {
		return false;
	}
	GumboOutput *output = gumbo_parse(str->ptr);
	if (output == NULL) {
		FAIL("Couldn't parse HTML successfully!");
		free_string(str);
		return false;
	}
	dump_html(output->root, text, text_line, &html_pos);
	gumbo_destroy_output(&kGumboDefaultOptions, output);
	free_string(str);
	return true;
}
