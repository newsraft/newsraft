#include <stdlib.h>
#include <string.h>
#include "render_data/render_data.h"

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
	struct line *target;
};

struct html_table_element_handler {
	const GumboTag tag_id;
	void (*start_handler)(struct html_table *, GumboVector *);
	void (*end_handler)(struct html_table *, GumboVector *);
	config_entry_id color_setting;
};

static void
expand_html_table_by_one_row(struct html_table *table, GumboVector *attrs)
{
	(void)attrs;
	struct html_table_row *tmp = realloc(table->rows, sizeof(struct html_table_row) * (table->rows_count + 1));
	if (tmp != NULL) {
		tmp[table->rows_count].cells = NULL;
		tmp[table->rows_count].cells_count = 0;
		tmp[table->rows_count].max_cell_height = 1;
		table->rows = tmp;
		table->rows_count += 1;
	}
}

static int8_t
get_cells_count_from_tag_attribute(GumboVector *attrs, const char *attr_name)
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
expand_last_row_in_html_table_by_one_cell(struct html_table *table, GumboVector *attrs)
{
	if (table->rows_count == 0) {
		expand_html_table_by_one_row(table, attrs);
		if (table->rows_count == 0) {
			return;
		}
	}

	int64_t i;
	struct html_table_row *last_row = &table->rows[table->rows_count - 1];
	int64_t old_count = last_row->cells_count;

	// Before adding another cell we have to check if the place where we write
	// the cell to is not reserved by the cells with the rowspan attribute
	// above. If it is reserved, we have to shift write target to another cell.
	for (i = 0; i < table->rows_count - 1; ++i) {
		if (last_row->cells_count < table->rows[i].cells_count) {
			if (table->rows[i].cells[last_row->cells_count].rowspan + i >= table->rows_count) {
				last_row->cells_count += 1;
				i = -1; // And again.
			}
		}
	}

	int8_t colspan = get_cells_count_from_tag_attribute(attrs, "colspan");
	int8_t rowspan = get_cells_count_from_tag_attribute(attrs, "rowspan");

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
	table->target = &tmp[last_row->cells_count - colspan].line;
}

static void
expand_target_cell_by_one_line(struct html_table *table, GumboVector *attrs)
{
	(void)attrs;
	if (table->target != NULL) {
		line_bump(table->target);
	}
}

static void
write_to_table(struct html_table *table, const char *src, size_t src_len)
{
	// Without converting multi byte text turns into gibberish
	struct wstring *wsrc = convert_array_to_wstring(src, src_len);
	if (wsrc != NULL) {
		if (table->target != NULL) {
			// You may want to remove leading whitespace here but think again
			// It's called not only for table elements but for everything else
			line_string(table->target, wsrc->ptr);
		} else {
			wcatss(table->unmapped_text, wsrc);
		}
		free_wstring(wsrc);
	}
}

static inline void
free_html_table(const struct html_table *table)
{
	for (int i = 0; i < table->rows_count; ++i) {
		for (int j = 0; j < table->rows[i].cells_count; ++j) {
			free_render_result(&table->rows[i].cells[j].text);
		}
		free(table->rows[i].cells);
	}
	free(table->rows);
	free(table->column_widths);
	free_wstring(table->unmapped_text);
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
	if (table->unmapped_text->len > 0) {
		line_string(line, table->unmapped_text->ptr);
		line_char(line, L'\n');
	}

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

// TODO: add support for colgroup and col elements
static const struct html_table_element_handler handlers[] = {
	{GUMBO_TAG_TD,      &expand_last_row_in_html_table_by_one_cell, NULL, 0},
	{GUMBO_TAG_TR,      &expand_html_table_by_one_row,              NULL, 0},
	{GUMBO_TAG_TH,      &expand_last_row_in_html_table_by_one_cell, NULL, 0},
	{GUMBO_TAG_BR,      &expand_target_cell_by_one_line,            NULL, 0},
	// Gumbo can assign thead, tbody and tfoot tags to table element under
	// some circumstances, so we need to ignore these to avoid tag display.
	{GUMBO_TAG_THEAD,   NULL,                                       NULL, 0},
	{GUMBO_TAG_TBODY,   NULL,                                       NULL, 0},
	{GUMBO_TAG_TFOOT,   NULL,                                       NULL, 0},
	{GUMBO_TAG_B,       NULL,                                       NULL, CFG_COLOR_HTML_B},
	{GUMBO_TAG_I,       NULL,                                       NULL, CFG_COLOR_HTML_I},
	{GUMBO_TAG_U,       NULL,                                       NULL, CFG_COLOR_HTML_U},
	{GUMBO_TAG_UNKNOWN, NULL,                                       NULL, 0},
};

static void
dump_html_table_child(GumboNode *node, struct html_table *table)
{
	if (node->type == GUMBO_NODE_ELEMENT) {
		size_t i = 0, j = 0;
		while ((node->v.element.tag != handlers[i].tag_id) && (handlers[i].tag_id != GUMBO_TAG_UNKNOWN)) {
			i += 1;
		}
		if (handlers[i].tag_id == GUMBO_TAG_UNKNOWN) {
			write_to_table(table, node->v.element.original_tag.data, node->v.element.original_tag.length);
			while (j < node->v.element.children.length) {
				dump_html_table_child(node->v.element.children.data[j++], table);
			}
			write_to_table(table, node->v.element.original_end_tag.data, node->v.element.original_end_tag.length);
		} else {
			if (handlers[i].start_handler != NULL) {
				handlers[i].start_handler(table, &node->v.element.attributes);
			}
			if (handlers[i].color_setting > 0 && table->target != NULL) {
				table->target->style |= COLOR_TO_BIT(handlers[i].color_setting);
			}
			while (j < node->v.element.children.length) {
				dump_html_table_child(node->v.element.children.data[j++], table);
			}
			if (handlers[i].color_setting > 0 && table->target != NULL) {
				table->target->style &= ~COLOR_TO_BIT(handlers[i].color_setting);
			}
			if (handlers[i].end_handler != NULL) {
				handlers[i].end_handler(table, &node->v.element.attributes);
			}
		}
	} else if ((node->type == GUMBO_NODE_TEXT) || (node->type == GUMBO_NODE_CDATA)) {
		write_to_table(table, node->v.text.text, strlen(node->v.text.text));
	}
}

void
write_contents_of_html_table_node_to_text(struct line *line, GumboNode *node)
{
	struct html_table table = {0};
	table.unmapped_text = wcrtes(10);
	if (table.unmapped_text == NULL) {
		return;
	}
	for (size_t i = 0; i < node->v.element.children.length; ++i) {
		dump_html_table_child(node->v.element.children.data[i], &table);
	}
	print_html_table(line, &table);
	free_html_table(&table);
}
