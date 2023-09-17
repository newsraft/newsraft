#include <stdlib.h>
#include <string.h>
#include "render_data/render_data.h"

struct html_table_cell {
	struct wstring **lines;
	size_t lines_count;
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
	struct html_table_cell *target;
};

struct html_table_element_handler {
	const GumboTag tag_id;
	void (*start_handler)(struct html_table *, GumboVector *);
	void (*end_handler)(struct html_table *, GumboVector *);
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
	if ((attr != NULL) && (attr->value != NULL)) {
		int8_t result;
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

	for (i = old_count; i < (last_row->cells_count - colspan); ++i) {
		tmp[i].lines = NULL;
		tmp[i].colspan = 1; // TODO borrow colspan from cells above
		tmp[i].rowspan = -1;
		tmp[i].index = i;
	}
	for (i = last_row->cells_count - colspan + 1; i < last_row->cells_count; ++i) {
		tmp[i].lines = NULL;
		tmp[i].colspan = -1;
		tmp[i].rowspan = rowspan;
		tmp[i].index = i;
	}

	tmp[last_row->cells_count - colspan].lines = malloc(sizeof(struct wstring *));
	tmp[last_row->cells_count - colspan].lines[0] = wcrtes(10);
	tmp[last_row->cells_count - colspan].lines_count = 1;
	tmp[last_row->cells_count - colspan].colspan = colspan;
	tmp[last_row->cells_count - colspan].rowspan = rowspan;
	tmp[last_row->cells_count - colspan].index = last_row->cells_count - colspan;
	last_row->cells = tmp;

	if (last_row->cells_count > table->columns_count) {
		int64_t prev_columns_count = table->columns_count;
		table->columns_count = last_row->cells_count;
		size_t *tmp2 = realloc(table->column_widths, sizeof(size_t) * table->columns_count);
		if (tmp2 != NULL) {
			table->column_widths = tmp2;
			for (i = prev_columns_count; i < table->columns_count; ++i) {
				table->column_widths[i] = 0;
			}
		}
	}

	table->target = &tmp[last_row->cells_count - colspan];
}

static void
expand_target_cell_by_one_line(struct html_table *table, GumboVector *attrs)
{
	(void)attrs;
	if (table->target != NULL) {
		struct wstring **tmp = realloc(
			table->target->lines,
			sizeof(struct wstring *) * (table->target->lines_count + 1)
		);
		if (tmp != NULL) {
			tmp[table->target->lines_count] = wcrtes(10);
			table->target->lines = tmp;
			table->target->lines_count += 1;
		}
		struct html_table_row *last_row = &table->rows[table->rows_count - 1];
		if (last_row->max_cell_height < table->target->lines_count) {
			last_row->max_cell_height = table->target->lines_count;
		}
	}
}

static void
write_to_table(struct html_table *table, const char *src, size_t src_len)
{
	struct string *tag = crtas(src, src_len);
	if (tag == NULL) {
		return;
	}
	trim_whitespace_from_string(tag);
	inlinefy_string(tag);
	struct wstring *wtag = convert_string_to_wstring(tag);
	free_string(tag);
	if (wtag == NULL) {
		return;
	}
	if (table->target != NULL) {
		struct wstring *last_line = table->target->lines[table->target->lines_count - 1];
		wcatss(last_line, wtag);
		// Don't expand width of an entire column just because of the content
		// of a cell that spans multiple columns. The required width will be
		// calculated later to evenly space the content of cells that are not
		// spread across multiple columns.
		if (table->target->colspan == 1) {
			if (table->column_widths[table->target->index] < last_line->len) {
				table->column_widths[table->target->index] = last_line->len;
			}
		}
	} else {
		wcatss(table->unmapped_text, wtag);
	}
	free_wstring(wtag);
}

static inline void
free_html_table(const struct html_table *table)
{
	for (int64_t i = 0; i < table->rows_count; ++i) {
		for (int64_t j = 0; j < table->rows[i].cells_count; ++j) {
			if (table->rows[i].cells[j].lines != NULL) {
				for (size_t k = 0; k < table->rows[i].cells[j].lines_count; ++k) {
					free_wstring(table->rows[i].cells[j].lines[k]);
				}
			}
			free(table->rows[i].cells[j].lines);
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
					if (sum_width < table->rows[i].cells[j].lines[0]->len) {
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
print_html_table(struct line *line, const struct html_table *table)
{
	wchar_t buf[8192];
	size_t w;
	if (table->unmapped_text->len > 0) {
		line_string(line, table->unmapped_text->ptr);
		line_char(line, L'\n');
	}
	for (int64_t i = 0; i < table->rows_count; ++i) {
		for (size_t height = 0; height < table->rows[i].max_cell_height; ++height) {
			for (int64_t j = 0; j < table->rows[i].cells_count; ++j) {
				if (table->rows[i].cells[j].colspan > 0) {
					w = table->column_widths[j];
					for (int64_t k = j + 1; (k < table->rows[i].cells_count) && (table->rows[i].cells[k].colspan == -1); ++k) {
						w += table->column_widths[k] + 1;
					}
					if ((table->rows[i].cells[j].rowspan == -1)
						|| (height >= table->rows[i].cells[j].lines_count))
					{
						swprintf(buf, 8192, L"%-*.*ls ", w, w, L"");
					} else {
						swprintf(buf, 8192, L"%-*.*ls ", w, w, table->rows[i].cells[j].lines[height]->ptr);
					}
					line_string(line, buf);
				}
			}
			line_char(line, L'\n');
		}
	}
}

static const struct html_table_element_handler handlers[] = {
	{GUMBO_TAG_TD,      &expand_last_row_in_html_table_by_one_cell, NULL},
	{GUMBO_TAG_TR,      &expand_html_table_by_one_row,              NULL},
	{GUMBO_TAG_TH,      &expand_last_row_in_html_table_by_one_cell, NULL},
	{GUMBO_TAG_BR,      &expand_target_cell_by_one_line,            NULL},
	// Gumbo can assign thead, tbody and tfoot tags to table element under
	// some circumstances, so we need to ignore these to avoid tag display.
	{GUMBO_TAG_THEAD,   NULL,                                       NULL},
	{GUMBO_TAG_TBODY,   NULL,                                       NULL},
	{GUMBO_TAG_TFOOT,   NULL,                                       NULL},
	{GUMBO_TAG_UNKNOWN, NULL,                                       NULL},
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
			while (j < node->v.element.children.length) {
				dump_html_table_child(node->v.element.children.data[j++], table);
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
	adjust_column_widths_to_contain_cells_with_colspan_attribute(&table);
	print_html_table(line, &table);
	free_html_table(&table);
}
