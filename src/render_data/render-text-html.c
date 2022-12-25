#include <string.h>
#include "render_data.h"

#define MAX_NESTED_LISTS_DEPTH 10
#define SPACES_PER_INDENTATION_LEVEL 4

enum html_position {
	HTML_NONE = 0,
	HTML_PRE = 1,
};

enum list_type {
	UNORDERED_LIST,
	ORDERED_LIST,
};

struct html_element_renderer {
	const GumboTag tag_id;
	void (*start_handler)(struct line *, enum html_position *);
	void (*end_handler)(struct line *, enum html_position *);
};

struct list_level {
	enum list_type type;
	uint16_t length;
};

static uint8_t list_depth;
// We are keeping first element of the list_levels for the list items
// which were placed without the beginning of the listing (ul, ol).
static struct list_level list_levels[MAX_NESTED_LISTS_DEPTH + 1];

static inline void
provide_newlines(struct line *line, size_t count)
{
	for (size_t i = 0; (i < line->target->len) && (line->target->ptr[line->target->len - i - 1] == L'\n'); ++i) {
		count = count > 0 ? count - 1 : 0;
	}
	for (size_t i = 0; i < count; ++i) {
		line_char(line, L'\n');
	}
}

static void
provide_one_newline(struct line *line, enum html_position *pos)
{
	(void)pos;
	provide_newlines(line, 1);
}

static void
provide_two_newlines(struct line *line, enum html_position *pos)
{
	(void)pos;
	provide_newlines(line, 2);
}

static void
br_handler(struct line *line, enum html_position *pos)
{
	(void)pos;
	line_char(line, L'\n');
}

static void
hr_handler(struct line *line, enum html_position *pos)
{
	size_t temp_indent = line->indent;
	line->indent = 0;
	provide_one_newline(line, pos);
	for (size_t i = 1; i < line->lim; ++i) {
		line_char(line, L'─');
	}
	line_char(line, L'\n');
	line->indent = temp_indent;
}

static void
li_handler(struct line *line, enum html_position *pos)
{
	provide_one_newline(line, pos);
	line->indent = list_depth * SPACES_PER_INDENTATION_LEVEL;
	if (list_levels[list_depth].type == UNORDERED_LIST) {
		line_string(line, L"*  ");
		line->indent += 3;
	} else {
		++(list_levels[list_depth].length);
		// 9 = 5 (for longest uint16_t) + 2 (for dot and space) + 1 (for terminator) + 1 (for luck lol)
		wchar_t number_str[9];
		size_t number_str_len = swprintf(number_str, 9, L"%d. ", list_levels[list_depth].length);
		line_string(line, number_str);
		line->indent += number_str_len;
	}
}

static void
ul_start_handler(struct line *line, enum html_position *pos)
{
	(void)pos;
	if (list_depth != MAX_NESTED_LISTS_DEPTH) {
		provide_newlines(line, list_depth == 0 ? 2 : 1);
		list_levels[++list_depth].type = UNORDERED_LIST;
	}
}

static void
ul_end_handler(struct line *line, enum html_position *pos)
{
	(void)pos;
	if (list_depth > 0) {
		--list_depth;
		line->indent = list_depth * SPACES_PER_INDENTATION_LEVEL;
	}
	provide_newlines(line, list_depth == 0 ? 2 : 1);
}

static void
ol_start_handler(struct line *line, enum html_position *pos)
{
	(void)pos;
	if (list_depth != MAX_NESTED_LISTS_DEPTH) {
		provide_newlines(line, list_depth == 0 ? 2 : 1);
		++list_depth;
		list_levels[list_depth].type = ORDERED_LIST;
		list_levels[list_depth].length = 0;
	}
}

static void
figure_start_handler(struct line *line, enum html_position *pos)
{
	provide_two_newlines(line, pos);
	line->indent += SPACES_PER_INDENTATION_LEVEL;
}

static void
figure_end_handler(struct line *line, enum html_position *pos)
{
	provide_two_newlines(line, pos);
	if (line->indent >= SPACES_PER_INDENTATION_LEVEL) {
		line->indent -= SPACES_PER_INDENTATION_LEVEL;
	}
}

static void
dd_start_handler(struct line *line, enum html_position *pos)
{
	provide_one_newline(line, pos);
	line->indent += SPACES_PER_INDENTATION_LEVEL;
}

static void
dd_end_handler(struct line *line, enum html_position *pos)
{
	provide_one_newline(line, pos);
	if (line->indent >= SPACES_PER_INDENTATION_LEVEL) {
		line->indent -= SPACES_PER_INDENTATION_LEVEL;
	}
}

static void
pre_start_handler(struct line *line, enum html_position *pos)
{
	provide_two_newlines(line, pos);
	*pos |= HTML_PRE;
}

static void
pre_end_handler(struct line *line, enum html_position *pos)
{
	provide_two_newlines(line, pos);
	*pos &= ~HTML_PRE;
}

static void
add_bold_style(struct line *line, enum html_position *pos)
{
	(void)pos;
	append_format_hint_to_line(line, FORMAT_BOLD_BEGIN);
}

static void
remove_bold_style(struct line *line, enum html_position *pos)
{
	(void)pos;
	append_format_hint_to_line(line, FORMAT_BOLD_END);
}

static void
add_italic_style(struct line *line, enum html_position *pos)
{
	(void)pos;
	append_format_hint_to_line(line, FORMAT_ITALIC_BEGIN);
}

static void
remove_italic_style(struct line *line, enum html_position *pos)
{
	(void)pos;
	append_format_hint_to_line(line, FORMAT_ITALIC_END);
}

static void
add_underlined_style(struct line *line, enum html_position *pos)
{
	(void)pos;
	append_format_hint_to_line(line, FORMAT_UNDERLINED_BEGIN);
}

static void
remove_underlined_style(struct line *line, enum html_position *pos)
{
	(void)pos;
	append_format_hint_to_line(line, FORMAT_UNDERLINED_END);
}

static void
header_start_handler(struct line *line, enum html_position *pos)
{
	(void)pos;
	provide_newlines(line, 3);
	append_format_hint_to_line(line, FORMAT_BOLD_BEGIN);
}

static void
header_end_handler(struct line *line, enum html_position *pos)
{
	append_format_hint_to_line(line, FORMAT_BOLD_END);
	provide_two_newlines(line, pos);
}

static const struct html_element_renderer renderers[] = {
	{GUMBO_TAG_P,          &provide_two_newlines, &provide_two_newlines},
	{GUMBO_TAG_I,          &add_italic_style,     &remove_italic_style},
	{GUMBO_TAG_B,          &add_bold_style,       &remove_bold_style},
	{GUMBO_TAG_U,          &add_underlined_style, &remove_underlined_style},
	{GUMBO_TAG_EM,         &add_italic_style,     &remove_italic_style},
	{GUMBO_TAG_VAR,        &add_italic_style,     &remove_italic_style},
	{GUMBO_TAG_STRONG,     &add_bold_style,       &remove_bold_style},
	{GUMBO_TAG_DL,         &provide_two_newlines, &provide_two_newlines},
	{GUMBO_TAG_BR,         &br_handler,           NULL},
	{GUMBO_TAG_LI,         &li_handler,           &provide_one_newline},
	{GUMBO_TAG_UL,         &ul_start_handler,     &ul_end_handler},
	{GUMBO_TAG_OL,         &ol_start_handler,     &ul_end_handler},
	{GUMBO_TAG_PRE,        &pre_start_handler,    &pre_end_handler},
	{GUMBO_TAG_H1,         &header_start_handler, &header_end_handler},
	{GUMBO_TAG_H2,         &header_start_handler, &header_end_handler},
	{GUMBO_TAG_H3,         &header_start_handler, &header_end_handler},
	{GUMBO_TAG_H4,         &header_start_handler, &header_end_handler},
	{GUMBO_TAG_H5,         &header_start_handler, &header_end_handler},
	{GUMBO_TAG_H6,         &header_start_handler, &header_end_handler},
	{GUMBO_TAG_HR,         &hr_handler,           NULL},
	{GUMBO_TAG_FIGURE,     &figure_start_handler, &figure_end_handler},
	{GUMBO_TAG_BLOCKQUOTE, &figure_start_handler, &figure_end_handler},
	{GUMBO_TAG_DD,         &dd_start_handler,     &dd_end_handler},
	{GUMBO_TAG_DT,         &provide_one_newline,  &provide_one_newline},
	{GUMBO_TAG_DIV,        &provide_one_newline,  &provide_one_newline},
	{GUMBO_TAG_CENTER,     &provide_one_newline,  &provide_one_newline},
	{GUMBO_TAG_MAIN,       &provide_one_newline,  &provide_one_newline},
	{GUMBO_TAG_ARTICLE,    &provide_one_newline,  &provide_one_newline},
	{GUMBO_TAG_SUMMARY,    &provide_one_newline,  &provide_one_newline},
	{GUMBO_TAG_DETAILS,    &provide_two_newlines, &provide_two_newlines},
	{GUMBO_TAG_ADDRESS,    &provide_one_newline,  &provide_one_newline},
	{GUMBO_TAG_FIGCAPTION, &provide_one_newline,  &provide_one_newline},
	{GUMBO_TAG_SECTION,    &provide_one_newline,  &provide_one_newline},
	{GUMBO_TAG_FOOTER,     &provide_one_newline,  &provide_one_newline},
	{GUMBO_TAG_OPTION,     &provide_one_newline,  &provide_one_newline},
	{GUMBO_TAG_FORM,       &provide_one_newline,  &provide_one_newline},
	{GUMBO_TAG_ASIDE,      &provide_one_newline,  &provide_one_newline},
	{GUMBO_TAG_NAV,        &provide_one_newline,  &provide_one_newline},
	{GUMBO_TAG_TABLE,      NULL,                  NULL},
	{GUMBO_TAG_UNKNOWN,    NULL,                  NULL},
};

static void
dump_html(GumboNode *node, struct line *line, enum html_position *pos)
{
	if (node->type == GUMBO_NODE_ELEMENT) {
		size_t i = 0;
		while ((renderers[i].tag_id != GUMBO_TAG_UNKNOWN) && (renderers[i].tag_id != node->v.element.tag)) {
			i += 1;
		}
		if (renderers[i].tag_id == GUMBO_TAG_UNKNOWN) {
			struct wstring *tag = convert_array_to_wstring(node->v.element.original_tag.data, node->v.element.original_tag.length);
			if (tag != NULL) {
				line_string(line, tag->ptr);
				free_wstring(tag);
			}
			for (size_t j = 0; j < node->v.element.children.length; ++j) {
				dump_html(node->v.element.children.data[j], line, pos);
			}
			tag = convert_array_to_wstring(node->v.element.original_end_tag.data, node->v.element.original_end_tag.length);
			if (tag != NULL) {
				line_string(line, tag->ptr);
				free_wstring(tag);
			}
		} else if (renderers[i].tag_id == GUMBO_TAG_TABLE) {
			write_contents_of_html_table_node_to_text(line, node);
		} else {
			if (renderers[i].start_handler != NULL) {
				renderers[i].start_handler(line, pos);
			}
			for (size_t j = 0; j < node->v.element.children.length; ++j) {
				dump_html(node->v.element.children.data[j], line, pos);
			}
			if (renderers[i].end_handler != NULL) {
				renderers[i].end_handler(line, pos);
			}
		}
	} else if ((node->type == GUMBO_NODE_TEXT)
		|| (node->type == GUMBO_NODE_CDATA)
		|| (node->type == GUMBO_NODE_WHITESPACE))
	{
		struct wstring *wstr = convert_array_to_wstring(node->v.text.text, strlen(node->v.text.text));
		if (wstr != NULL) {
			for (const wchar_t *i = wstr->ptr; *i != L'\0'; ++i) {
				if (!ISWIDEWHITESPACE(*i)) {
					line_char(line, *i);
				} else if ((*pos & HTML_PRE) != 0) {
					if (*i == '\n') {
						line_char(line, L'\n');
					} else if (*i == '\t') {
						line_string(line, L"    ");
					} else {
						line_char(line, L' ');
					}
				} else if (line->target->len > 0) {
					if (!ISWIDEWHITESPACE(line->target->ptr[line->target->len - 1])) {
						line_char(line, L' ');
					}
				}
			}
			free_wstring(wstr);
		}
	}
}

bool
render_text_html(struct line *line, const struct wstring *source)
{
	enum html_position html_pos = HTML_NONE;
	list_depth = 0;
	list_levels[0].type = UNORDERED_LIST;
	list_levels[0].length = 0;
	struct string *str = convert_wstring_to_string(source);
	if (str == NULL) {
		return false;
	}
	GumboOutput *output = gumbo_parse(str->ptr);
	if (output == NULL) {
		FAIL("Couldn't parse HTML successfully!");
		free_string(str);
		return false;
	}
	dump_html(output->root, line, &html_pos);
	gumbo_destroy_output(&kGumboDefaultOptions, output);
	free_string(str);
	return true;
}
