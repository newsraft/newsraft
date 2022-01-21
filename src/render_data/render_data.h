#ifndef RENDER_DATA_H
#define RENDER_DATA_H
#include "feedeater.h"

struct line {
	wchar_t *ptr;  // Line text.
	size_t len;    // Shows actual number of characters in ptr.
	size_t lim;    // Shows how many characters can fit in ptr.
	size_t pin;    // Holds index of the last space in ptr.
	size_t indent; // Shows how many spaces must be printed in the beginning of ptr.
};

struct html_table_cell {
	struct wstring *value;
	size_t hspan;          // horizontal span
	size_t vspan;          // vertical span
};

struct html_table_row {
	struct html_table_cell *cells;
	size_t width;                  // number of cells in the row
	size_t height;                 // max height of the cell with (vspan == 1) in row
};

struct html_table {
	struct html_table_row *rows;
	size_t rows_count;           // number of rows in table
	size_t columns_count;
	size_t *column_widths;       // max widths of cell per column
	struct line cell_line;
	struct wstring *cell_text;
};

bool render_text_plain(const struct wstring *source, struct line *line, struct wstring *target, bool is_first_call);
bool render_text_html(const struct wstring *source, struct line *line, struct wstring *target, bool is_first_call);

bool line_char(struct line *line, wchar_t c, struct wstring *target);
bool line_string(struct line *line, const wchar_t *str, struct wstring *target);

// html table
struct html_table *create_html_table(void);
bool expand_html_table_by_one_row(struct html_table *table);
bool expand_last_row_in_html_table_by_one_cell(struct html_table *table);
void print_html_table(struct html_table *table);
void free_html_table(struct html_table *table);
#endif // RENDER_DATA_H
