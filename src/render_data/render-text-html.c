#include <string.h>
#include "render_data.h"

#define MAX_NESTED_LISTS_DEPTH 10
#define SPACES_PER_INDENTATION_LEVEL 4
#define HTML_TABLE_COLUMN_SPACING 2

struct html_table_cell {
	struct render_result text;
	struct line line;
	int8_t colspan;
	int8_t rowspan;
	int64_t index;
};

struct html_table_row {
	struct html_table_cell *cells;
	int64_t cells_count;
	size_t max_cell_height;
};

struct html_table {
	struct html_table_row *rows;
	int64_t rows_count;
	int64_t columns_count;
	size_t *column_widths;
	struct wstring *unmapped_text; // text outside of cells
};

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

	bool in_table; // Nested tables are not supported!
	struct html_table table;
};

struct html_element_renderer {
	GumboTag tag_id;
	void (*start_handler)(struct html_render *, GumboVector *);
	void (*end_handler)(struct html_render *, GumboVector *);
	config_entry_id color_setting;
};

static void
br_handler(struct html_render *ctx, GumboVector *attrs)
{
	(void)attrs;
	line_char(ctx->line, L'\n');
}

static void
hr_handler(struct html_render *ctx, GumboVector *attrs)
{
	(void)attrs;
	ctx->line->head->indent = 0;
	for (size_t i = 0; i < ctx->line->lim; ++i) {
		line_char(ctx->line, L'─');
	}
}

static void
li_handler(struct html_render *ctx, GumboVector *attrs)
{
	(void)attrs;
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
ul_start_handler(struct html_render *ctx, GumboVector *attrs)
{
	(void)attrs;
	if (ctx->list_depth < MAX_NESTED_LISTS_DEPTH) {
		ctx->list_depth += 1;
		ctx->line->head->indent = ctx->list_depth * SPACES_PER_INDENTATION_LEVEL;
		ctx->line->next_indent = ctx->line->head->indent;
		ctx->list_levels[ctx->list_depth].is_ordered = false;
	}
}

static void
ul_end_handler(struct html_render *ctx, GumboVector *attrs)
{
	(void)attrs;
	if (ctx->list_depth > 0) {
		ctx->list_depth -= 1;
		ctx->line->head->indent = ctx->list_depth * SPACES_PER_INDENTATION_LEVEL;
		ctx->line->next_indent = ctx->line->head->indent;
	}
}

static void
ol_start_handler(struct html_render *ctx, GumboVector *attrs)
{
	(void)attrs;
	if (ctx->list_depth < MAX_NESTED_LISTS_DEPTH) {
		ctx->list_depth += 1;
		ctx->line->head->indent = ctx->list_depth * SPACES_PER_INDENTATION_LEVEL;
		ctx->line->next_indent = ctx->line->head->indent;
		ctx->list_levels[ctx->list_depth].is_ordered = true;
		ctx->list_levels[ctx->list_depth].length = 0;
	}
}

static void
indent_enter_handler(struct html_render *ctx, GumboVector *attrs)
{
	(void)attrs;
	ctx->line->head->indent += SPACES_PER_INDENTATION_LEVEL;
	ctx->line->next_indent = ctx->line->head->indent;
}

static void
indent_leave_handler(struct html_render *ctx, GumboVector *attrs)
{
	(void)attrs;
	if (ctx->line->head->indent >= SPACES_PER_INDENTATION_LEVEL) {
		ctx->line->next_indent = ctx->line->head->indent - SPACES_PER_INDENTATION_LEVEL;
	}
	if (ctx->line->head->ws->len == 0) {
		ctx->line->head->indent = ctx->line->next_indent;
	}
}

static void
html_table_add_row(struct html_render *ctx, GumboVector *attrs)
{
	(void)attrs;
	struct html_table_row *tmp = realloc(ctx->table.rows, sizeof(struct html_table_row) * (ctx->table.rows_count + 1));
	if (tmp != NULL) {
		tmp[ctx->table.rows_count].cells = NULL;
		tmp[ctx->table.rows_count].cells_count = 0;
		tmp[ctx->table.rows_count].max_cell_height = 1;
		ctx->table.rows = tmp;
		ctx->table.rows_count += 1;
	}
}

static int8_t
html_table_get_cells_count_from_tag_attribute(GumboVector *attrs, const char *attr_name)
{
	GumboAttribute *attr = gumbo_get_attribute(attrs, attr_name);
	if (attr != NULL && attr->value != NULL) {
		int8_t result = 0;
		if (sscanf(attr->value, "%" SCNd8, &result) == 1) {
			return result > 0 ? result : 1;
		}
	}
	return 1;
}

static void
html_table_add_cell(struct html_render *ctx, GumboVector *attrs)
{
	if (ctx->table.rows_count == 0) {
		html_table_add_row(ctx, attrs);
		if (ctx->table.rows_count == 0) {
			return;
		}
	}

	int64_t i;
	struct html_table_row *last_row = &ctx->table.rows[ctx->table.rows_count - 1];
	int64_t old_count = last_row->cells_count;

	// Before adding another cell we have to check if the place where we write
	// the cell to is not reserved by the cells with the rowspan attribute
	// above. If it is reserved, we have to shift write target to another cell.
	for (i = 0; i < ctx->table.rows_count - 1; ++i) {
		if (last_row->cells_count < ctx->table.rows[i].cells_count) {
			if (ctx->table.rows[i].cells[last_row->cells_count].rowspan + i >= ctx->table.rows_count) {
				last_row->cells_count += 1;
				i = -1; // And again.
			}
		}
	}

	int8_t colspan = html_table_get_cells_count_from_tag_attribute(attrs, "colspan");
	int8_t rowspan = html_table_get_cells_count_from_tag_attribute(attrs, "rowspan");

	last_row->cells_count += colspan;

	struct html_table_cell *tmp = realloc(last_row->cells, sizeof(struct html_table_cell) * last_row->cells_count);
	if (tmp == NULL) {
		return;
	}

	for (i = old_count; i < last_row->cells_count; ++i) {
		memset(&tmp[i], 0, sizeof(struct html_table_cell));
		tmp[i].line.target = &tmp[i].text;
		if (i < last_row->cells_count - colspan) {
			tmp[i].colspan = 1; // TODO borrow colspan from cells above
			tmp[i].rowspan = -1;
		} else if (i > last_row->cells_count - colspan) {
			tmp[i].colspan = -1;
			tmp[i].rowspan = rowspan;
		} else {
			tmp[i].colspan = colspan;
			tmp[i].rowspan = rowspan;
		}
		tmp[i].index = i;
		line_bump(&tmp[i].line);
	}

	last_row->cells = tmp;
	ctx->line = &tmp[last_row->cells_count - colspan].line;
}

static inline void
adjust_column_widths_to_contain_cells_with_colspan_attribute(struct html_table *table)
{
	size_t sum_width;
	size_t *min_width;
	for (int64_t i = 0; i < table->rows_count; ++i) {
		for (int64_t j = 0; j < table->rows[i].cells_count; ++j) {
			if (table->rows[i].cells[j].colspan > 1) {
				while (true) {
					sum_width = table->column_widths[j];
					min_width = &table->column_widths[j];
					for (int64_t k = 1; (k < table->rows[i].cells[j].colspan) && ((j + k) < table->columns_count); ++k) {
						sum_width += table->column_widths[j + k];
						if (table->column_widths[j + k] < *min_width) {
							min_width = &table->column_widths[j + k];
						}
					}
					// table->rows[i].cells[j].lines is not NULL because
					// all cells with (colspan > 1) have at least one line.
					if (sum_width < table->rows[i].cells[j].text.lines[0].ws->len) {
						*min_width += 1;
					} else {
						break;
					}
				}
			}
		}
	}
}

static inline void
print_html_table(struct line *line, struct html_table *table)
{
	// Get maximum columns count
	int max_columns_count = 0;
	for (int i = 0; i < table->rows_count; ++i) {
		if (table->rows[i].cells_count > max_columns_count) {
			max_columns_count = table->rows[i].cells_count;
		}
	}
	table->columns_count = max_columns_count;
	table->column_widths = calloc(table->columns_count, sizeof(size_t));

	// Get row heights
	for (int i = 0; i < table->rows_count; ++i) {
		for (int j = 0; j < table->rows[i].cells_count; ++j) {
			if (table->rows[i].max_cell_height < table->rows[i].cells[j].text.lines_len) {
				table->rows[i].max_cell_height = table->rows[i].cells[j].text.lines_len;
			}
		}
	}

	// Get column widths
	for (int i = 0; i < table->rows_count; ++i) {
		for (int j = 0; j < table->rows[i].cells_count; ++j) {
			if (table->rows[i].cells[j].colspan > 1) {
				// Don't expand width of an entire column just because of the content
				// of a cell that spans multiple columns. The required width will be
				// calculated later to evenly space the content of cells that are not
				// spread across multiple columns.
				continue;
			}
			for (size_t k = 0; k < table->rows[i].cells[j].text.lines_len; ++k) {
				if (table->column_widths[j] < table->rows[i].cells[j].text.lines[k].ws->len) {
					table->column_widths[j] = table->rows[i].cells[j].text.lines[k].ws->len;
				}
			}
		}
	}

	adjust_column_widths_to_contain_cells_with_colspan_attribute(table);

	for (int64_t i = 0; i < table->rows_count; ++i) {
		// fprintf(stderr, "--> ROW\n");

		for (size_t height = 0; height < table->rows[i].max_cell_height; ++height) {
			for (int64_t j = 0; j < table->rows[i].cells_count; ++j) {
				// fprintf(stderr, "----> CEL (%zu)\n", height);

				struct html_table_cell *cells = &table->rows[i].cells[j];
				if (cells->colspan > 0) {
					long w = table->column_widths[j];
					for (int64_t k = j + 1; (k < table->rows[i].cells_count) && (table->rows[i].cells[k].colspan == -1); ++k) {
						w += HTML_TABLE_COLUMN_SPACING + table->column_widths[k];
					}
					if (cells->rowspan >= 0 && height < cells->text.lines_len) {
						// fprintf(stderr, "------> %ls\n", cells->text.lines[height].ws->ptr);
						for (size_t g = 0; g < cells->text.lines[height].ws->len; ++g) {
							line->style = cells->text.lines[height].hints[g];
							line_char(line, cells->text.lines[height].ws->ptr[g]);
						}
						w -= cells->text.lines[height].ws->len;
					}
					line->style = 0;

					// Be careful not to add whitespace for trailing column!
					if (j + 1 < table->rows[i].cells_count) {
						for (long l = 0; l < w + HTML_TABLE_COLUMN_SPACING; ++l) {
							line_string(line, L" "); // It's &nbsp;
						}
					}
				}

				// fprintf(stderr, "<---- CEL (%zu)\n", height);
			}
			line_char(line, L'\n');
		}

		// fprintf(stderr, "<-- ROW\n");
	}
}

static inline void
free_html_table(struct html_table *table)
{
	for (int i = 0; i < table->rows_count; ++i) {
		for (int j = 0; j < table->rows[i].cells_count; ++j) {
			free_render_result(&table->rows[i].cells[j].text);
		}
		free(table->rows[i].cells);
	}
	free(table->rows);
	free(table->column_widths);
	memset(table, 0, sizeof(struct html_table));
}

static const struct html_element_renderer renderers[] = {
	{GUMBO_TAG_BR,         &br_handler,           NULL,                  0},
	{GUMBO_TAG_B,          NULL,                  NULL,                  CFG_COLOR_HTML_B},
	{GUMBO_TAG_BIG,        NULL,                  NULL,                  CFG_COLOR_HTML_B},
	{GUMBO_TAG_U,          NULL,                  NULL,                  CFG_COLOR_HTML_U},
	{GUMBO_TAG_I,          NULL,                  NULL,                  CFG_COLOR_HTML_I},
	{GUMBO_TAG_EM,         NULL,                  NULL,                  CFG_COLOR_HTML_EM},
	{GUMBO_TAG_VAR,        NULL,                  NULL,                  CFG_COLOR_HTML_EM},
	{GUMBO_TAG_SMALL,      NULL,                  NULL,                  CFG_COLOR_HTML_EM},
	{GUMBO_TAG_DFN,        NULL,                  NULL,                  CFG_COLOR_HTML_EM},
	{GUMBO_TAG_INS,        NULL,                  NULL,                  CFG_COLOR_HTML_EM},
	{GUMBO_TAG_MARK,       NULL,                  NULL,                  CFG_COLOR_HTML_MARK},
	{GUMBO_TAG_STRONG,     NULL,                  NULL,                  CFG_COLOR_HTML_STRONG},
	{GUMBO_TAG_LI,         &li_handler,           NULL,                  0},
	{GUMBO_TAG_UL,         &ul_start_handler,     &ul_end_handler,       0},
	{GUMBO_TAG_OL,         &ol_start_handler,     &ul_end_handler,       0},
	{GUMBO_TAG_HR,         &hr_handler,           NULL,                  0},
	{GUMBO_TAG_FIGURE,     &indent_enter_handler, &indent_leave_handler, 0},
	{GUMBO_TAG_TD,         &html_table_add_cell,  NULL,                  0},
	{GUMBO_TAG_TR,         &html_table_add_row,   NULL,                  0},
	{GUMBO_TAG_TH,         &html_table_add_cell,  NULL,                  0},

	// Gumbo can assign thead, tbody and tfoot tags to table element under
	// some circumstances, so we need to ignore these to avoid tag display.
	{GUMBO_TAG_THEAD,      NULL,                  NULL,                  0},
	{GUMBO_TAG_TBODY,      NULL,                  NULL,                  0},
	{GUMBO_TAG_TFOOT,      NULL,                  NULL,                  0},

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
		if (node->v.element.tag == GUMBO_TAG_TABLE && ctx->in_table == false) {
			ctx->in_table = true;
			struct line *origin = ctx->line;
			for (size_t j = 0; j < node->v.element.children.length; ++j) {
				render_html(node->v.element.children.data[j], ctx);
			}
			print_html_table(origin, &ctx->table);
			free_html_table(&ctx->table);
			ctx->line = origin;
			ctx->in_table = false;
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
				renderers[i].start_handler(ctx, &node->v.element.attributes);
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
				renderers[i].end_handler(ctx, &node->v.element.attributes);
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
