#include <string.h>
#include <wctype.h>
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

	struct links_list *links;

	bool in_table;       // Nested tables are not supported
	size_t in_pre_depth; // Shows how many pre elements we are in

	struct string **abbrs;
	size_t abbrs_len;

	struct html_table table;
};

struct html_element_renderer {
	GumboTag tag_id;
	bool descentable;
	uint8_t newlines_before;
	uint8_t newlines_after;
	wchar_t *prefix;
	wchar_t *suffix;
	void (*start_handler)(struct html_render *, GumboVector *);
	void (*end_handler)(struct html_render *, GumboVector *);
	newsraft_video_t video_attrs;
};

static void
provide_newlines(struct html_render *ctx, size_t count, bool force)
{
	if (force == false) {
		size_t digits_in_the_beginning = 0;
		if (ctx->line->target->lines_len > 0) {
			for (size_t i = 0; i < ctx->line->target->lines[ctx->line->target->lines_len - 1].ws->len; ++i) {
				if (iswdigit(ctx->line->target->lines[ctx->line->target->lines_len - 1].ws->ptr[i])) {
					digits_in_the_beginning += 1;
				} else {
					break;
				}
			}
		}
		if ((digits_in_the_beginning > 0
			&& ctx->line->target->lines[ctx->line->target->lines_len - 1].ws->ptr[digits_in_the_beginning] == L'.')
			|| (ctx->line->target->lines_len > 0
			&& ctx->line->target->lines[ctx->line->target->lines_len - 1].ws->ptr[0] == L'*'))
		{
			return; // Ignore beginning of lists
		}
	}
	size_t empty_lines = 0;
	for (size_t i = ctx->line->target->lines_len; true; --i) {
		if (i == 0) {
			return;
		} else if (ctx->line->target->lines[i - 1].ws->len == 0) {
			empty_lines += 1;
		} else {
			break;
		}
	}
	if (count > empty_lines) {
		for (int i = count - empty_lines; i > 0; --i) {
			line_bump(ctx->line);
		}
	}
}

static const char *
get_value_of_xml_attribute(GumboVector *attrs, const char *attr_name)
{
	GumboAttribute *attr = gumbo_get_attribute(attrs, attr_name);
	return attr != NULL && attr->value != NULL ? attr->value : "";
}

static void
url_mark_handler(struct html_render *ctx, GumboVector *attrs)
{
	const char *links[] = {"href", "src", "data"};
	const char *names[] = {"title", "name", "alt"};
	const char *types[] = {"type"};
	const char *link = NULL;
	const char *type = NULL;
	const char *name = NULL;
	for (size_t l = 0; l < LENGTH(links) && link == NULL; ++l) {
		const char *a = get_value_of_xml_attribute(attrs, links[l]);
		if (strlen(a) > 0) link = a;
	}
	for (size_t t = 0; t < LENGTH(types) && type == NULL; ++t) {
		const char *a = get_value_of_xml_attribute(attrs, types[t]);
		if (strlen(a) > 0) type = a;
	}
	for (size_t n = 0; n < LENGTH(names) && name == NULL; ++n) {
		const char *a = get_value_of_xml_attribute(attrs, names[n]);
		if (strlen(a) > 0) name = a;
	}
	if (link    == NULL) return; // Ignore empty links
	if (link[0] == '#')  return; // Ignore anchors to elements
	size_t link_len = strlen(link);
	if (link_len == 0)   return; // Ignore empty links
	int64_t link_index = add_url_to_links_list(ctx->links, link, link_len);
	if (link_index < 0)  return; // Ignore invalid links

	wchar_t index[100];
	int index_len = swprintf(index, 100, L"[%" PRId64, link_index + 1);
	if (index_len < 2 || index_len > 99) return; // Should never happen

	if (ctx->line->head->ws->len > 0) {
		line_string(ctx->line, L" "); // It's &nbsp;
	}

	line_style(ctx->line, A_BOLD);

	line_string(ctx->line, index);

	if (type != NULL || name != NULL) {
		line_string(ctx->line, L", ");
		if (type != NULL) {
			struct wstring *w = convert_array_to_wstring(type, strlen(type));
			if (w != NULL) {
				line_string(ctx->line, w->ptr);
				free_wstring(w);
			}
		}
		if (name != NULL) {
			line_string(ctx->line, type == NULL ? L"\"" : L" \"");
			struct wstring *w = convert_array_to_wstring(name, strlen(name));
			if (w != NULL) {
				line_string(ctx->line, w->ptr);
				free_wstring(w);
			}
			line_char(ctx->line, L'"');
		}
	}
	line_char(ctx->line, L']');

	line_unstyle(ctx->line);
}

static void
abbr_handler(struct html_render *ctx, GumboVector *attrs)
{
	const char *title = get_value_of_xml_attribute(attrs, "title");
	if (title == NULL) {
		return;
	}
	const size_t title_len = strlen(title);
	if (title_len == 0) {
		return;
	}
	for (size_t i = 0; i < ctx->abbrs_len; ++i) {
		if (strcasecmp(title, ctx->abbrs[i]->ptr) == 0) {
			return; // It's a duplicate
		}
	}
	line_string(ctx->line, L" "); // It's &nbsp;
	line_char(ctx->line, L'(');
	struct wstring *w = convert_array_to_wstring(title, title_len);
	if (w != NULL) {
		line_string(ctx->line, w->ptr);
		free_wstring(w);
	}
	line_char(ctx->line, L')');
	void *tmp = realloc(ctx->abbrs, sizeof(struct string *) * (ctx->abbrs_len + 1));
	if (tmp != NULL) {
		ctx->abbrs = tmp;
		ctx->abbrs[ctx->abbrs_len] = crtas(title, title_len);
		if (ctx->abbrs[ctx->abbrs_len] != NULL) {
			ctx->abbrs_len += 1;
		}
	}
}

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
input_handler(struct html_render *ctx, GumboVector *attrs)
{
	if (strcmp(get_value_of_xml_attribute(attrs, "type"), "hidden") != 0) {
		const char *value = get_value_of_xml_attribute(attrs, "value");
		size_t value_len = strlen(value);
		if (value_len > 0) {
			struct wstring *w = convert_array_to_wstring(value, value_len);
			if (w != NULL) {
				line_char(ctx->line, L'[');
				line_string(ctx->line, w->ptr);
				line_char(ctx->line, L']');
				free_wstring(w);
			}
		}
	}
}

static void
li_handler(struct html_render *ctx, GumboVector *attrs)
{
	(void)attrs;
	if (ctx->list_depth > 0) {
		ctx->line->head->indent = SPACES_PER_INDENTATION_LEVEL * (ctx->list_depth * 2 - 1);
		ctx->line->indent = SPACES_PER_INDENTATION_LEVEL * (ctx->list_depth * 2);
	}
	if (ctx->list_levels[ctx->list_depth].is_ordered == true) {
		ctx->list_levels[ctx->list_depth].length += 1;
		wchar_t num[100];
		swprintf(num, 100, L"%u.  ", ctx->list_levels[ctx->list_depth].length);
		line_string(ctx->line, num);
	} else {
		line_string(ctx->line, L"*   ");
	}
}

static void
ul_start_handler(struct html_render *ctx, GumboVector *attrs)
{
	(void)attrs;
	if (ctx->list_depth < MAX_NESTED_LISTS_DEPTH) {
		ctx->list_depth += 1;
		ctx->list_levels[ctx->list_depth].is_ordered = false;
	}
}

static void
ul_end_handler(struct html_render *ctx, GumboVector *attrs)
{
	(void)attrs;
	if (ctx->list_depth > 0) {
		ctx->list_depth -= 1;
	}
	ctx->line->indent = SPACES_PER_INDENTATION_LEVEL * (ctx->list_depth * 2);
	ctx->line->head->indent = ctx->line->indent;
}

static void
ol_start_handler(struct html_render *ctx, GumboVector *attrs)
{
	long list_start = 0;
	GumboAttribute *start = gumbo_get_attribute(attrs, "start");
	if (start != NULL && start->value != NULL && strlen(start->value) > 0) {
		list_start = strtol(start->value, NULL, 10) - 1;
	}
	if (ctx->list_depth < MAX_NESTED_LISTS_DEPTH) {
		ctx->list_depth += 1;
		ctx->list_levels[ctx->list_depth].is_ordered = true;
		ctx->list_levels[ctx->list_depth].length = list_start;
	}
}
static void
indent_start_handler(struct html_render *ctx, GumboVector *attrs)
{
	(void)attrs;
	ctx->line->head->indent += SPACES_PER_INDENTATION_LEVEL;
	ctx->line->indent = ctx->line->head->indent;
}

static void
indent_end_handler(struct html_render *ctx, GumboVector *attrs)
{
	(void)attrs;
	if (ctx->line->head->indent >= SPACES_PER_INDENTATION_LEVEL) {
		ctx->line->indent = ctx->line->head->indent - SPACES_PER_INDENTATION_LEVEL;
	}
	if (ctx->line->head->ws->len == 0) {
		ctx->line->head->indent = ctx->line->indent;
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
							line_style(line, cells->text.lines[height].hints[g]);
							line_char(line, cells->text.lines[height].ws->ptr[g]);
							line_unstyle(line);
						}
						w -= cells->text.lines[height].ws->len;
					}

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
free_abbrs(struct html_render *ctx)
{
	for (size_t i = 0; i < ctx->abbrs_len; ++i) {
		free_string(ctx->abbrs[i]);
	}
	free(ctx->abbrs);
}

static inline void
free_html_table(struct html_table *table)
{
	for (int i = 0; i < table->rows_count; ++i) {
		for (int j = 0; j < table->rows[i].cells_count; ++j) {
			free(table->rows[i].cells[j].line.style_stack);
			free_render_result(&table->rows[i].cells[j].text);
		}
		free(table->rows[i].cells);
	}
	free(table->rows);
	free(table->column_widths);
	memset(table, 0, sizeof(struct html_table));
}

static const struct html_element_renderer renderers[] = {
	{GUMBO_TAG_SPAN,       true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_P,          true,  2, 2, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_BR,         true,  0, 0, NULL, NULL, &br_handler,           NULL,                A_NORMAL},
	{GUMBO_TAG_HR,         true,  1, 1, NULL, NULL, &hr_handler,           NULL,                A_NORMAL},
	{GUMBO_TAG_A,          true,  0, 0, NULL, NULL, NULL,                  &url_mark_handler,   A_UNDERLINE},
	{GUMBO_TAG_IMG,        true,  0, 0, NULL, L"[image]",  NULL,           &url_mark_handler,   A_UNDERLINE},
	{GUMBO_TAG_IFRAME,     true,  0, 0, NULL, L"[iframe]", NULL,           &url_mark_handler,   A_UNDERLINE},
	{GUMBO_TAG_EMBED,      true,  0, 0, NULL, L"[embed]",  NULL,           &url_mark_handler,   A_UNDERLINE},
	{GUMBO_TAG_SOURCE,     true,  0, 0, NULL, L"[source]", NULL,           &url_mark_handler,   A_UNDERLINE},
	{GUMBO_TAG_OBJECT,     true,  0, 0, NULL, L"[object]", NULL,           &url_mark_handler,   A_UNDERLINE},
	{GUMBO_TAG_AUDIO,      true,  0, 0, NULL, L"[audio]",  NULL,           &url_mark_handler,   A_UNDERLINE},
	{GUMBO_TAG_VIDEO,      true,  0, 0, NULL, L"[video]",  NULL,           &url_mark_handler,   A_UNDERLINE},
	{GUMBO_TAG_U,          true,  0, 0, NULL, NULL, NULL,                  NULL,                A_UNDERLINE},
	{GUMBO_TAG_B,          true,  0, 0, NULL, NULL, NULL,                  NULL,                A_BOLD},
	{GUMBO_TAG_BIG,        true,  0, 0, NULL, NULL, NULL,                  NULL,                A_BOLD},
	{GUMBO_TAG_I,          true,  0, 0, NULL, NULL, NULL,                  NULL,                NEWSRAFT_ITALIC},
	{GUMBO_TAG_EM,         true,  0, 0, NULL, NULL, NULL,                  NULL,                NEWSRAFT_ITALIC},
	{GUMBO_TAG_VAR,        true,  0, 0, NULL, NULL, NULL,                  NULL,                NEWSRAFT_ITALIC},
	{GUMBO_TAG_SMALL,      true,  0, 0, NULL, NULL, NULL,                  NULL,                NEWSRAFT_ITALIC},
	{GUMBO_TAG_DFN,        true,  0, 0, NULL, NULL, NULL,                  NULL,                NEWSRAFT_ITALIC},
	{GUMBO_TAG_INS,        true,  0, 0, NULL, NULL, NULL,                  NULL,                NEWSRAFT_ITALIC},
	{GUMBO_TAG_ADDRESS,    true,  1, 1, NULL, NULL, NULL,                  NULL,                NEWSRAFT_ITALIC},
	{GUMBO_TAG_MARK,       true,  0, 0, NULL, NULL, NULL,                  NULL,                A_BOLD|NEWSRAFT_ITALIC},
	{GUMBO_TAG_STRONG,     true,  0, 0, NULL, NULL, NULL,                  NULL,                A_BOLD},
	{GUMBO_TAG_HEADER,     true,  3, 2, NULL, NULL, NULL,                  NULL,                A_BOLD},
	{GUMBO_TAG_H1,         true,  3, 2, NULL, NULL, NULL,                  NULL,                A_BOLD},
	{GUMBO_TAG_H2,         true,  3, 2, NULL, NULL, NULL,                  NULL,                A_BOLD},
	{GUMBO_TAG_H3,         true,  3, 2, NULL, NULL, NULL,                  NULL,                A_BOLD},
	{GUMBO_TAG_H4,         true,  3, 2, NULL, NULL, NULL,                  NULL,                A_BOLD},
	{GUMBO_TAG_H5,         true,  3, 2, NULL, NULL, NULL,                  NULL,                A_BOLD},
	{GUMBO_TAG_H6,         true,  3, 2, NULL, NULL, NULL,                  NULL,                A_BOLD},
	{GUMBO_TAG_PRE,        true,  1, 1, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_LI,         true,  1, 1, NULL, NULL, &li_handler,           NULL,                A_NORMAL},
	{GUMBO_TAG_UL,         true,  0, 0, NULL, NULL, &ul_start_handler,     &ul_end_handler,     A_NORMAL},
	{GUMBO_TAG_OL,         true,  0, 0, NULL, NULL, &ol_start_handler,     &ul_end_handler,     A_NORMAL},
	{GUMBO_TAG_FIGURE,     true,  2, 2, NULL, NULL, &indent_start_handler, &indent_end_handler, A_NORMAL},
	{GUMBO_TAG_BLOCKQUOTE, true,  2, 2, NULL, NULL, &indent_start_handler, &indent_end_handler, A_NORMAL},
	{GUMBO_TAG_DD,         true,  1, 1, NULL, NULL, &indent_start_handler, &indent_end_handler, A_NORMAL},
	{GUMBO_TAG_TD,         true,  0, 0, NULL, NULL, &html_table_add_cell,  NULL,                A_NORMAL},
	{GUMBO_TAG_TH,         true,  0, 0, NULL, NULL, &html_table_add_cell,  NULL,                A_NORMAL},
	{GUMBO_TAG_TR,         true,  0, 0, NULL, NULL, &html_table_add_row,   NULL,                A_NORMAL},
	{GUMBO_TAG_ABBR,       true,  0, 0, NULL, NULL, NULL,                  &abbr_handler,       A_NORMAL},
	{GUMBO_TAG_INPUT,      true,  0, 0, NULL, NULL, &input_handler,        NULL,                A_NORMAL},
	{GUMBO_TAG_DIV,        true,  1, 1, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_CENTER,     true,  1, 1, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_MAIN,       true,  1, 1, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_ARTICLE,    true,  1, 1, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_SUMMARY,    true,  1, 1, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_FIGCAPTION, true,  1, 1, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_SECTION,    true,  1, 1, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_FOOTER,     true,  1, 1, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_OPTION,     true,  1, 1, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_FORM,       true,  1, 1, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_ASIDE,      true,  1, 1, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_NAV,        true,  1, 1, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_HGROUP,     true,  1, 1, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_DT,         true,  1, 1, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_DL,         true,  2, 2, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_DETAILS,    true,  2, 2, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_LABEL,      true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_TEXTAREA,   true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_CODE,       true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_TT,         true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_SAMP,       true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_KBD,        true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_CITE,       true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_TIME,       true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_FONT,       true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_BASEFONT,   true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_THEAD,      true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_TBODY,      true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_TFOOT,      true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_WBR,        true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_NOSCRIPT,   true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_DATA,       true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_APPLET,     true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_PARAM,      true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_LINK,       true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_CANVAS,     true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_META,       true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_BODY,       true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_HTML,       true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_SUP,        true,  0, 0, L"^",  NULL, NULL,                 NULL,                A_NORMAL},
	{GUMBO_TAG_Q,          true,  0, 0, L"\"", L"\"", NULL,                NULL,                A_NORMAL},
	{GUMBO_TAG_BUTTON,     true,  0, 0, L"[",  L"]",  NULL,                NULL,                A_NORMAL},
	{GUMBO_TAG_SVG,        false, 0, 0, NULL, L" [svg image]", NULL,       NULL,                A_NORMAL},
	{GUMBO_TAG_HEAD,       false, 0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_STYLE,      false, 0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_SCRIPT,     false, 0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	// {GUMBO_TAG_PICTURE, true,  0, 0, NULL, NULL, NULL, /* not gumbo */  NULL,                A_NORMAL},

	// Gumbo can assign thead, tbody and tfoot tags to table element under
	// some circumstances, so we need to ignore these to avoid tag display.
	{GUMBO_TAG_THEAD,      true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_TBODY,      true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
	{GUMBO_TAG_TFOOT,      true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},

	{GUMBO_TAG_UNKNOWN,    true,  0, 0, NULL, NULL, NULL,                  NULL,                A_NORMAL},
};

static void
render_html(GumboNode *node, struct html_render *ctx)
{
	if (node->type == GUMBO_NODE_TEXT || node->type == GUMBO_NODE_CDATA || node->type == GUMBO_NODE_WHITESPACE) {
		struct wstring *wstr = convert_array_to_wstring(node->v.text.text, strlen(node->v.text.text));
		if (wstr == NULL) {
			return;
		}
		for (const wchar_t *i = wstr->ptr; *i != L'\0'; ++i) {
			if (ctx->in_pre_depth > 0) {
				line_char(ctx->line, *i);
			} else if (!ISWIDEWHITESPACE(*i)) {
				line_char(ctx->line, *i);
			} else if (ctx->line->head->ws->len > 0) {
				if (!ISWIDEWHITESPACE(ctx->line->head->ws->ptr[ctx->line->head->ws->len - 1])) {
					line_char(ctx->line, L' ');
				}
			}
		}
		free_wstring(wstr);
	} else if (node->type == GUMBO_NODE_ELEMENT) {
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
			if (renderers[i].tag_id == GUMBO_TAG_UL || renderers[i].tag_id == GUMBO_TAG_OL) {
				provide_newlines(ctx, ctx->list_depth == 0 ? 2 : 1, true);
			} else {
				provide_newlines(ctx, renderers[i].newlines_before, false);
			}
			if (renderers[i].tag_id == GUMBO_TAG_PRE) {
				ctx->in_pre_depth += 1;
			}
			if (renderers[i].start_handler != NULL) {
				renderers[i].start_handler(ctx, &node->v.element.attributes);
			}
			if (renderers[i].video_attrs != A_NORMAL) {
				line_style(ctx->line, renderers[i].video_attrs);
			}
			line_string(ctx->line, renderers[i].prefix);
			if (renderers[i].descentable == true) {
				for (size_t j = 0; j < node->v.element.children.length; ++j) {
					render_html(node->v.element.children.data[j], ctx);
				}
			}
			line_string(ctx->line, renderers[i].suffix);
			if (renderers[i].video_attrs != A_NORMAL) {
				line_unstyle(ctx->line);
			}
			if (renderers[i].end_handler != NULL) {
				renderers[i].end_handler(ctx, &node->v.element.attributes);
			}
			if (renderers[i].tag_id == GUMBO_TAG_PRE) {
				ctx->in_pre_depth -= 1;
			}
			if (renderers[i].tag_id == GUMBO_TAG_UL || renderers[i].tag_id == GUMBO_TAG_OL) {
				provide_newlines(ctx, ctx->list_depth == 0 ? 2 : 1, true);
			} else {
				provide_newlines(ctx, renderers[i].newlines_after, true);
			}
		}
	}
}

bool
render_text_html(struct line *line, const struct wstring *source, struct links_list *links)
{
	struct html_render html = {
		.line  = line,
		.links = links,
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
	free_abbrs(&html);
	return true;
}
