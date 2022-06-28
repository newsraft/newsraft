#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "render_data/render_data.h"

// This HTML table renderer needs to be rewritten because it's kind of creepy :|

struct html_table_cell {
	struct wstring **lines;
	int64_t lines_count;
	int8_t colspan;
	int8_t rowspan;
};

struct html_table_row {
	struct html_table_cell *cells;
	int64_t cells_count;
};

struct html_table {
	struct html_table_row *rows;
	int64_t rows_count;            // number of rows in table
	int64_t columns_count;         // max cells_count of rows
	int64_t *column_widths;        // max widths of cell per column
	struct wstring *unmapped_text; // text outside of cells
	struct wstring *target;
};

struct html_table_element_handler {
	const GumboTag tag_id;
	void (*start_handler)(struct html_table *, GumboVector *);
	void (*end_handler)(struct html_table *, GumboVector *);
};

struct html_table *
create_html_table(void)
{
	struct html_table *table = malloc(sizeof(struct html_table));
	if (table == NULL) {
		return NULL;
	}
	table->unmapped_text = wcrtes();
	if (table->unmapped_text == NULL) {
		free(table);
		return NULL;
	}
	table->rows = NULL;
	table->rows_count = 0;
	table->columns_count = 0;
	table->column_widths = NULL;
	table->target = table->unmapped_text;
	return table;
}

static void
append_empty_cell_to_last_row_of_the_table(struct html_table *table)
{
	table->rows[table->rows_count - 1].cells_count += 1;
	struct html_table_cell *tmp = realloc(
		table->rows[table->rows_count - 1].cells,
		sizeof(struct html_table_cell) * table->rows[table->rows_count - 1].cells_count
	);
	if (tmp == NULL) {
		return;
	}
	tmp[table->rows[table->rows_count - 1].cells_count - 1].lines = malloc(sizeof(struct wstring *));
	tmp[table->rows[table->rows_count - 1].cells_count - 1].lines_count = 1;
	tmp[table->rows[table->rows_count - 1].cells_count - 1].lines[0] = wcrtes();
	tmp[table->rows[table->rows_count - 1].cells_count - 1].colspan = 1;
	tmp[table->rows[table->rows_count - 1].cells_count - 1].rowspan = 1;
	table->rows[table->rows_count - 1].cells = tmp;
}

static void
expand_html_table_by_one_row(struct html_table *table, GumboVector *attrs)
{
	(void)attrs;
	struct html_table_row *temp = realloc(table->rows, sizeof(struct html_table_row) * (table->rows_count + 1));
	if (temp != NULL) {
		temp[table->rows_count].cells = NULL;
		temp[table->rows_count].cells_count = 0;
		table->rows = temp;
		table->rows_count += 1;
	}
	// Populate new row with empty columns that we get from rowspan of columns from previous rows.
	// Note that we do it only for the first consecutive columns with rowspan attribute.
	for (int64_t i = 0; i < table->rows_count - 1; ++i) {
		for (int64_t j = 0; j < table->rows[i].cells_count; ++j) {
			if (table->rows[i].cells[j].rowspan == 1) {
				break;
			}
			if (table->rows[i].cells[j].rowspan - table->rows_count + i >= 0) {
				append_empty_cell_to_last_row_of_the_table(table);
			}
		}
	}
}

static int8_t
get_int8_t_of_xml_attribute(GumboVector *attrs, const char *attr_name)
{
	GumboAttribute *attr = gumbo_get_attribute(attrs, attr_name);
	if (attr != NULL) {
		if (attr->value != NULL) {
			int8_t result;
			if (sscanf(attr->value, "%" SCNd8, &result) == 1) {
				return result;
			}
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

	bool filled_reserved_space = false;
	int64_t index;
	// Before adding another cell we have to check if the place where we write
	// the cell to is not reserved by the cells with the rowspan attribute
	// above. If it is reserved, we have to iteratively fill reserved space
	// with empty cells.
	while (filled_reserved_space == false) {
		filled_reserved_space = true;
		index = table->rows[table->rows_count - 1].cells_count;
		for (int64_t i = 0; i < table->rows_count - 1; ++i) {
			if (index < table->rows[i].cells_count) {
				if (table->rows[i].cells[index].rowspan - table->rows_count + i >= 0) {
					filled_reserved_space = false;
					append_empty_cell_to_last_row_of_the_table(table);
				}
			}
		}
	}

	int8_t colspan = get_int8_t_of_xml_attribute(attrs, "colspan");
	struct html_table_cell *temp = realloc(table->rows[table->rows_count - 1].cells, sizeof(struct html_table_cell) * (index + colspan));
	if (temp == NULL) {
		return;
	}
	int8_t rowspan = get_int8_t_of_xml_attribute(attrs, "rowspan");
	for (int64_t i = index + 1; i < (index + colspan); ++i) {
		temp[i].lines = NULL;
		temp[i].colspan = 1;
		temp[i].rowspan = rowspan;
	}
	temp[index].lines = malloc(sizeof(struct wstring *));
	temp[index].lines[0] = wcrtes();
	temp[index].lines_count = 1;
	temp[index].colspan = colspan;
	temp[index].rowspan = rowspan;
	table->rows[table->rows_count - 1].cells = temp;
	table->rows[table->rows_count - 1].cells_count += colspan;
	if (table->rows[table->rows_count - 1].cells_count > table->columns_count) {
		int64_t prev_columns_count = table->columns_count;
		table->columns_count = table->rows[table->rows_count - 1].cells_count;
		int64_t *temp2 = realloc(table->column_widths, sizeof(int64_t) * table->columns_count);
		if (temp2 != NULL) {
			table->column_widths = temp2;
			for (int64_t i = prev_columns_count; i < table->columns_count; ++i) {
				table->column_widths[i] = 0;
			}
		}
	}
	table->target = temp[index].lines[0];
}

static void
write_to_table(struct html_table *table, const char *src, size_t src_len)
{
	struct string *tag = crtas(src, src_len);
	if (tag == NULL) {
		return;
	}
	struct wstring *wtag = convert_string_to_wstring(tag);
	free_string(tag);
	if (wtag == NULL) {
		return;
	}
	if (wcatss(table->target, wtag) == false) {
		free_wstring(wtag);
		return;
	}
	free_wstring(wtag);
	if ((table->rows_count > 0) && (table->rows[table->rows_count - 1].cells_count > 0)) {
		int64_t i;
		for (i = table->rows[table->rows_count - 1].cells_count - 1; i > 0; --i) {
			if (table->rows[table->rows_count - 1].cells[i].lines != NULL) {
				break;
			}
		}
		if (table->rows[table->rows_count - 1].cells[i].colspan == 1) {
			if ((size_t)(table->column_widths[i]) < table->target->len) {
				table->column_widths[i] = (int64_t)(table->target->len);
			}
		}
	}
}

static inline void
free_html_table(struct html_table *table)
{
	for (int64_t i = 0; i < table->rows_count; ++i) {
		for (int64_t j = 0; j < table->rows[i].cells_count; ++j) {
			if (table->rows[i].cells[j].lines != NULL) {
				free_wstring(table->rows[i].cells[j].lines[0]);
				free(table->rows[i].cells[j].lines);
			}
		}
		free(table->rows[i].cells);
	}
	free(table->rows);
	free(table->column_widths);
	free_wstring(table->unmapped_text);
	free(table);
}

static inline void
adjust_column_widths_to_contain_cells_with_colspan_attribute(struct html_table *table)
{
	int64_t sum_width;
	int64_t *min_width;
	for (int64_t i = 0; i < table->rows_count; ++i) {
		for (int64_t j = 0; j < table->rows[i].cells_count; ++j) {
			if (table->rows[i].cells[j].colspan > 1) {
				while (true) {
					sum_width = 0;
					min_width = &table->column_widths[j];
					for (int64_t k = 0; (k < table->rows[i].cells[j].colspan) && ((j + k) < table->columns_count); ++k) {
						if (table->column_widths[j + k] < *min_width) {
							min_width = &table->column_widths[j + k];
						}
						sum_width += table->column_widths[j + k];
					}
					if ((size_t)sum_width < table->rows[i].cells[j].lines[0]->len) {
						*min_width += 1;
					} else {
						break;
					}
				}
			}
		}
	}
}

void
print_html_table_and_free_it(struct html_table *table, struct wstring *text, struct line *line)
{
	wchar_t buf[5000];
	size_t w;
	if (table->unmapped_text->len > 0) {
		line_string(line, table->unmapped_text->ptr, text);
		line_char(line, L'\n', text);
	}
	adjust_column_widths_to_contain_cells_with_colspan_attribute(table);
	for (int64_t i = 0; i < table->rows_count; ++i) {
		for (int64_t j = 0; j < table->rows[i].cells_count; ++j) {
			if (table->rows[i].cells[j].lines != NULL) {
				w = table->column_widths[j];
				if (table->rows[i].cells[j].colspan > 1) {
					for (int64_t k = j + 1; (k < table->rows[i].cells_count) && (table->rows[i].cells[k].lines == NULL); ++k) {
						w += table->column_widths[k] + 1;
					}
				}
				swprintf(buf, 5000, L"%-*.*ls ", w, w, table->rows[i].cells[j].lines[0]->ptr);
				line_string(line, buf, text);
			}
		}
		line_char(line, L'\n', text);
	}
	free_html_table(table);
}

static const struct html_table_element_handler handlers[] = {
	{GUMBO_TAG_TD,      &expand_last_row_in_html_table_by_one_cell, NULL},
	{GUMBO_TAG_TR,      &expand_html_table_by_one_row,              NULL},
	{GUMBO_TAG_TH,      &expand_last_row_in_html_table_by_one_cell, NULL},
	// Gumbo can assign thead, tbody and tfoot tags to table element under
	// some circumstances, so we need to ignore these to avoid tag display.
	{GUMBO_TAG_THEAD,   NULL,                                       NULL},
	{GUMBO_TAG_TBODY,   NULL,                                       NULL},
	{GUMBO_TAG_TFOOT,   NULL,                                       NULL},
	{GUMBO_TAG_UNKNOWN, NULL,                                       NULL},
};

void
dump_html_table(GumboNode *node, struct html_table *table)
{
	if (node->type == GUMBO_NODE_ELEMENT) {
		size_t i, j;
		for (i = 0; handlers[i].tag_id != GUMBO_TAG_UNKNOWN; ++i) {
			if (node->v.element.tag == handlers[i].tag_id) {
				break;
			}
		}
		if (handlers[i].tag_id == GUMBO_TAG_UNKNOWN) {
			write_to_table(table, node->v.element.original_tag.data, node->v.element.original_tag.length);
			for (j = 0; j < node->v.element.children.length; ++j) {
				dump_html_table(node->v.element.children.data[j], table);
			}
			write_to_table(table, node->v.element.original_end_tag.data, node->v.element.original_end_tag.length);
		} else {
			if (handlers[i].start_handler != NULL) {
				handlers[i].start_handler(table, &node->v.element.attributes);
			}
			for (j = 0; j < node->v.element.children.length; ++j) {
				dump_html_table(node->v.element.children.data[j], table);
			}
			if (handlers[i].end_handler != NULL) {
				handlers[i].end_handler(table, &node->v.element.attributes);
			}
		}
	} else if ((node->type == GUMBO_NODE_TEXT) || (node->type == GUMBO_NODE_CDATA)) {
		write_to_table(table, node->v.text.text, strlen(node->v.text.text));
	}
}
