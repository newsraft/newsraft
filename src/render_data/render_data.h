#ifndef RENDER_DATA_H
#define RENDER_DATA_H
#include <gumbo.h>
#include "newsraft.h"

struct line {
	struct wstring *target;     // Text buffer of actual data to be displayed.
	size_t len;                 // Length of the last text line.
	size_t lim;                 // Capacity of one text line.
	size_t pin;                 // Index of the last space in the last text line.
	size_t indent;              // Shows how many spaces must be in the beginning of line.
	struct format_hint **hints;
	size_t *hints_len;
};

bool render_text_html(struct line *line, const struct wstring *source);
void write_contents_of_html_table_node_to_text(struct line *line, GumboNode *node);

bool line_char(struct line *line, wchar_t c);
bool line_string(struct line *line, const wchar_t *str);
bool append_format_hint_to_line(struct line *line, format_hint_mask hint);
#endif // RENDER_DATA_H
