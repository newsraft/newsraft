#include <stdlib.h>
#include "render_data/render_data.h"

struct html_table *
create_html_table(void)
{
	struct html_table *table = malloc(sizeof(struct html_table));
	if (table == NULL) {
		return NULL;
	}
	table->rows = NULL;
	table->rows_count = 0;
	table->columns_count = 0;
	table->column_widths = NULL;
	table->cell_line.ptr = malloc(sizeof(wchar_t) * (list_menu_width + 1));
	if (table->cell_line.ptr == NULL) {
		FAIL("Not enough memory for line buffer to render data!");
		free(table);
		return NULL;
	}
	table->cell_line.len = 0;
	table->cell_line.lim = list_menu_width + 1;
	table->cell_line.pin = SIZE_MAX;
	table->cell_line.indent = 0;
	table->cell_text = NULL;
	return table;
}

bool
expand_html_table_by_one_row(struct html_table *table)
{
	struct html_table_row *temp = realloc(table->rows, sizeof(struct html_table_row) * (table->rows_count + 1));
	if (temp == NULL) {
		return false;
	}
	temp[table->rows_count].cells = NULL;
	temp[table->rows_count].width = 0;
	temp[table->rows_count].height = 1;
	table->rows = temp;
	++(table->rows_count);
	return true;
}

static bool
finish_off_previous_cell(struct html_table *table)
{
	if ((table->cell_text != NULL) && (table->cell_line.len != 0)) {
		for (size_t i = 0; i < table->cell_line.len; ++i) {
			wcatcs(table->cell_text, table->cell_line.ptr[i]);
		}
		table->cell_line.len = 0;
	}
	return true;
}

bool
expand_last_row_in_html_table_by_one_cell(struct html_table *table)
{
	finish_off_previous_cell(table);
	struct html_table_cell *temp = realloc(
		table->rows[table->rows_count - 1].cells,
		sizeof(struct html_table_cell) * (table->rows[table->rows_count - 1].width + 1)
	);
	if (temp == NULL) {
		return false;
	}
	temp[table->rows[table->rows_count - 1].width].value = wcrtes();
	temp[table->rows[table->rows_count - 1].width].hspan = 1;
	temp[table->rows[table->rows_count - 1].width].vspan = 1;
	table->rows[table->rows_count - 1].cells = temp;
	++(table->rows[table->rows_count - 1].width);
	if (table->rows[table->rows_count - 1].width > table->columns_count) {
		table->columns_count = table->rows[table->rows_count - 1].width;
		size_t *temp2 = realloc(table->column_widths, sizeof(size_t) * table->columns_count);
		if (temp2 == NULL) {
			return false;
		}
		temp2[table->columns_count - 1] = 0;
		table->column_widths = temp2;
	}
	table->cell_text = table->rows[table->rows_count - 1].cells[table->rows[table->rows_count - 1].width - 1].value;
	return true;
}

void
print_html_table(struct html_table *table)
{
	finish_off_previous_cell(table);
	for (size_t i = 0; i < table->rows_count; ++i) {
		for (size_t j = 0; j < table->rows[i].width; ++j) {
			if (table->rows[i].cells[j].value->len > table->column_widths[j]) {
				table->column_widths[j] = table->rows[i].cells[j].value->len;
			}
		}
	}
	for (size_t i = 0; i < table->rows_count; ++i) {
		for (size_t j = 0; j < table->rows[i].width; ++j) {
			/* fprintf(stderr, " %*ls |", table->column_widths[j], table->rows[i].cells[j].value->ptr); */
			fprintf(stderr, "%-*ls ", (int)table->column_widths[j], table->rows[i].cells[j].value->ptr);
		}
		fprintf(stderr, "\n");
	}
}

void
free_html_table(struct html_table *table)
{
	if (table == NULL) {
		return;
	}
	for (size_t i = 0; i < table->rows_count; ++i) {
		for (size_t j = 0; j < table->rows[i].width; ++j) {
			free_wstring(table->rows[i].cells[j].value);
		}
		free(table->rows[i].cells);
	}
	free(table->rows);
	free(table->column_widths);
	free(table->cell_line.ptr);
	free(table);
}
