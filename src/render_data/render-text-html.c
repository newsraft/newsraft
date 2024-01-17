#include <string.h>
#include "render_data.h"

#define MAX_NESTED_LISTS_DEPTH 10
#define SPACES_PER_INDENTATION_LEVEL 4

enum list_type {
	UNORDERED_LIST,
	ORDERED_LIST,
};

struct html_element_renderer {
	const GumboTag tag_id;
	void (*start_handler)(struct line *);
	void (*end_handler)(struct line *);
};

struct list_level {
	enum list_type type;
	uint16_t length;
};

static uint8_t list_depth;
// We are keeping first element of the list_levels for the list items
// which were placed without the beginning of the listing (ul, ol).
static struct list_level list_levels[MAX_NESTED_LISTS_DEPTH + 1];

static void
br_handler(struct line *line)
{
	line_char(line, L'\n');
}

static void
hr_handler(struct line *line)
{
	line->head->indent = 0;
	for (size_t i = 0; i < line->lim; ++i) {
		line_char(line, L'─');
	}
}

static void
li_handler(struct line *line)
{
	line->head->indent = list_depth * SPACES_PER_INDENTATION_LEVEL;
	if (list_levels[list_depth].type == UNORDERED_LIST) {
		line_string(line, L"*  ");
		line->next_indent = line->head->indent + 3;
	} else {
		list_levels[list_depth].length += 1;
		// 5 (for longest uint16_t) + 2 (for dot and space) + 1 (for terminator) + 12 (for luck lol) = 20
		wchar_t number_str[20];
		int number_str_len = swprintf(number_str, 20, L"%d. ", list_levels[list_depth].length);
		line_string(line, number_str);
		line->next_indent = line->head->indent + number_str_len;
	}
}

static void
ul_start_handler(struct line *line)
{
	if (list_depth < MAX_NESTED_LISTS_DEPTH) {
		list_depth += 1;
		line->head->indent = list_depth * SPACES_PER_INDENTATION_LEVEL;
		line->next_indent = line->head->indent;
		list_levels[list_depth].type = UNORDERED_LIST;
	}
}

static void
ul_end_handler(struct line *line)
{
	if (list_depth > 0) {
		list_depth -= 1;
		line->head->indent = list_depth * SPACES_PER_INDENTATION_LEVEL;
		line->next_indent = line->head->indent;
	}
}

static void
ol_start_handler(struct line *line)
{
	if (list_depth < MAX_NESTED_LISTS_DEPTH) {
		list_depth += 1;
		line->head->indent = list_depth * SPACES_PER_INDENTATION_LEVEL;
		line->next_indent = line->head->indent;
		list_levels[list_depth].type = ORDERED_LIST;
		list_levels[list_depth].length = 0;
	}
}

static void
indent_enter_handler(struct line *line)
{
	line->head->indent += SPACES_PER_INDENTATION_LEVEL;
	line->next_indent = line->head->indent;
}

static void
indent_leave_handler(struct line *line)
{
	if (line->head->indent >= SPACES_PER_INDENTATION_LEVEL) {
		line->head->indent -= SPACES_PER_INDENTATION_LEVEL;
		line->next_indent = line->head->indent;
	}
}

static void
add_bold_style(struct line *line)
{
	line_style(line, FORMAT_BOLD_BEGIN);
}

static void
remove_bold_style(struct line *line)
{
	line_style(line, FORMAT_BOLD_END);
}

static void
add_italic_style(struct line *line)
{
	line_style(line, FORMAT_ITALIC_BEGIN);
}

static void
remove_italic_style(struct line *line)
{
	line_style(line, FORMAT_ITALIC_END);
}

static void
add_underlined_style(struct line *line)
{
	line_style(line, FORMAT_UNDERLINED_BEGIN);
}

static void
remove_underlined_style(struct line *line)
{
	line_style(line, FORMAT_UNDERLINED_END);
}

static const struct html_element_renderer renderers[] = {
	{GUMBO_TAG_BR,         &br_handler,           NULL},
	{GUMBO_TAG_B,          &add_bold_style,       &remove_bold_style},
	{GUMBO_TAG_I,          &add_italic_style,     &remove_italic_style},
	{GUMBO_TAG_U,          &add_underlined_style, &remove_underlined_style},
	{GUMBO_TAG_LI,         &li_handler,           NULL},
	{GUMBO_TAG_UL,         &ul_start_handler,     &ul_end_handler},
	{GUMBO_TAG_OL,         &ol_start_handler,     &ul_end_handler},
	{GUMBO_TAG_HR,         &hr_handler,           NULL},
	{GUMBO_TAG_FIGURE,     &indent_enter_handler, &indent_leave_handler},
	{GUMBO_TAG_UNKNOWN,    NULL,                  NULL},
};

static void
render_html(GumboNode *node, struct line *line)
{
	if (node->type == GUMBO_NODE_ELEMENT) {
		size_t i = 0;
		while ((renderers[i].tag_id != GUMBO_TAG_UNKNOWN) && (renderers[i].tag_id != node->v.element.tag)) {
			i += 1;
		}
		if (node->v.element.tag == GUMBO_TAG_TABLE) {
			write_contents_of_html_table_node_to_text(line, node);
		} else if (renderers[i].tag_id == GUMBO_TAG_UNKNOWN) {
			struct wstring *tag = convert_array_to_wstring(node->v.element.original_tag.data, node->v.element.original_tag.length);
			if (tag != NULL) {
				line_string(line, tag->ptr);
				free_wstring(tag);
			}
			for (size_t j = 0; j < node->v.element.children.length; ++j) {
				render_html(node->v.element.children.data[j], line);
			}
			tag = convert_array_to_wstring(node->v.element.original_end_tag.data, node->v.element.original_end_tag.length);
			if (tag != NULL) {
				line_string(line, tag->ptr);
				free_wstring(tag);
			}
		} else {
			if (renderers[i].start_handler != NULL) {
				renderers[i].start_handler(line);
			}
			for (size_t j = 0; j < node->v.element.children.length; ++j) {
				render_html(node->v.element.children.data[j], line);
			}
			if (renderers[i].end_handler != NULL) {
				renderers[i].end_handler(line);
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
				} else if (line->head->ws->len > 0) {
					if (!ISWIDEWHITESPACE(line->head->ws->ptr[line->head->ws->len - 1])) {
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
	render_html(output->root, line);
	gumbo_destroy_output(&kGumboDefaultOptions, output);
	free_string(str);
	return true;
}
