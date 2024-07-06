#include <string.h>
#include "render_data.h"

#define MAX_NESTED_LISTS_DEPTH 10
#define SPACES_PER_INDENTATION_LEVEL 4

struct list_level {
	bool is_ordered;
	unsigned length;
};

struct html_render {
	struct line *line;
	uint8_t list_depth;

	// We are keeping first element of the list_levels for the list items
	// which were placed without the beginning of the listing (ul, ol).
	struct list_level list_levels[MAX_NESTED_LISTS_DEPTH + 1];
};

struct html_element_renderer {
	GumboTag tag_id;
	void (*start_handler)(struct html_render *);
	void (*end_handler)(struct html_render *);
	config_entry_id color_setting;
};

static void
br_handler(struct html_render *ctx)
{
	line_char(ctx->line, L'\n');
}

static void
hr_handler(struct html_render *ctx)
{
	ctx->line->head->indent = 0;
	for (size_t i = 0; i < ctx->line->lim; ++i) {
		line_char(ctx->line, L'─');
	}
}

static void
li_handler(struct html_render *ctx)
{
	ctx->line->head->indent = ctx->list_depth * SPACES_PER_INDENTATION_LEVEL;
	ctx->line->next_indent = ctx->line->head->indent + 3;
	if (ctx->list_levels[ctx->list_depth].is_ordered == true) {
		ctx->list_levels[ctx->list_depth].length += 1;
		wchar_t num[100];
		swprintf(num, 100, L"%u. ", ctx->list_levels[ctx->list_depth].length);
		line_string(ctx->line, num);
	} else {
		line_string(ctx->line, L"*  ");
	}
}

static void
ul_start_handler(struct html_render *ctx)
{
	if (ctx->list_depth < MAX_NESTED_LISTS_DEPTH) {
		ctx->list_depth += 1;
		ctx->line->head->indent = ctx->list_depth * SPACES_PER_INDENTATION_LEVEL;
		ctx->line->next_indent = ctx->line->head->indent;
		ctx->list_levels[ctx->list_depth].is_ordered = false;
	}
}

static void
ul_end_handler(struct html_render *ctx)
{
	if (ctx->list_depth > 0) {
		ctx->list_depth -= 1;
		ctx->line->head->indent = ctx->list_depth * SPACES_PER_INDENTATION_LEVEL;
		ctx->line->next_indent = ctx->line->head->indent;
	}
}

static void
ol_start_handler(struct html_render *ctx)
{
	if (ctx->list_depth < MAX_NESTED_LISTS_DEPTH) {
		ctx->list_depth += 1;
		ctx->line->head->indent = ctx->list_depth * SPACES_PER_INDENTATION_LEVEL;
		ctx->line->next_indent = ctx->line->head->indent;
		ctx->list_levels[ctx->list_depth].is_ordered = true;
		ctx->list_levels[ctx->list_depth].length = 0;
	}
}

static void
indent_enter_handler(struct html_render *ctx)
{
	ctx->line->head->indent += SPACES_PER_INDENTATION_LEVEL;
	ctx->line->next_indent = ctx->line->head->indent;
}

static void
indent_leave_handler(struct html_render *ctx)
{
	if (ctx->line->head->indent >= SPACES_PER_INDENTATION_LEVEL) {
		ctx->line->next_indent = ctx->line->head->indent - SPACES_PER_INDENTATION_LEVEL;
	}
	if (ctx->line->head->ws->len == 0) {
		ctx->line->head->indent = ctx->line->next_indent;
	}
}

static const struct html_element_renderer renderers[] = {
	{GUMBO_TAG_BR,         &br_handler,           NULL,                  0},
	{GUMBO_TAG_B,          NULL,                  NULL,                  CFG_COLOR_HTML_B},
	{GUMBO_TAG_I,          NULL,                  NULL,                  CFG_COLOR_HTML_I},
	{GUMBO_TAG_U,          NULL,                  NULL,                  CFG_COLOR_HTML_U},
	{GUMBO_TAG_LI,         &li_handler,           NULL,                  0},
	{GUMBO_TAG_UL,         &ul_start_handler,     &ul_end_handler,       0},
	{GUMBO_TAG_OL,         &ol_start_handler,     &ul_end_handler,       0},
	{GUMBO_TAG_HR,         &hr_handler,           NULL,                  0},
	{GUMBO_TAG_FIGURE,     &indent_enter_handler, &indent_leave_handler, 0},
	{GUMBO_TAG_UNKNOWN,    NULL,                  NULL,                  0},
};

static void
render_html(GumboNode *node, struct html_render *ctx)
{
	if (node->type == GUMBO_NODE_ELEMENT) {
		size_t i = 0;
		while ((renderers[i].tag_id != GUMBO_TAG_UNKNOWN) && (renderers[i].tag_id != node->v.element.tag)) {
			i += 1;
		}
		if (node->v.element.tag == GUMBO_TAG_TABLE) {
			write_contents_of_html_table_node_to_text(ctx->line, node);
		} else if (renderers[i].tag_id == GUMBO_TAG_UNKNOWN) {
			struct wstring *tag = convert_array_to_wstring(node->v.element.original_tag.data, node->v.element.original_tag.length);
			if (tag != NULL) {
				line_string(ctx->line, tag->ptr);
				free_wstring(tag);
			}
			for (size_t j = 0; j < node->v.element.children.length; ++j) {
				render_html(node->v.element.children.data[j], ctx);
			}
			tag = convert_array_to_wstring(node->v.element.original_end_tag.data, node->v.element.original_end_tag.length);
			if (tag != NULL) {
				line_string(ctx->line, tag->ptr);
				free_wstring(tag);
			}
		} else {
			if (renderers[i].start_handler != NULL) {
				renderers[i].start_handler(ctx);
			}
			if (renderers[i].color_setting > 0) {
				ctx->line->style |= COLOR_TO_BIT(renderers[i].color_setting);
			}
			for (size_t j = 0; j < node->v.element.children.length; ++j) {
				render_html(node->v.element.children.data[j], ctx);
			}
			if (renderers[i].color_setting > 0) {
				ctx->line->style &= ~COLOR_TO_BIT(renderers[i].color_setting);
			}
			if (renderers[i].end_handler != NULL) {
				renderers[i].end_handler(ctx);
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
					line_char(ctx->line, *i);
				} else if (ctx->line->head->ws->len > 0) {
					if (!ISWIDEWHITESPACE(ctx->line->head->ws->ptr[ctx->line->head->ws->len - 1])) {
						line_char(ctx->line, L' ');
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
	struct html_render html = {
		.line        = line,
		.list_depth  = 0,
		.list_levels = {{false, 0}},
	};
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
	render_html(output->root, &html);
	gumbo_destroy_output(&kGumboDefaultOptions, output);
	free_string(str);
	return true;
}
