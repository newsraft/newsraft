#ifndef RENDER_DATA_H
#define RENDER_DATA_H
#include <gumbo.h>
#include "newsraft.h"

struct line {
	struct wstring *target;     // Text buffer of actual data to be displayed.
	wchar_t *ptr;               // Intermediate buffer for one line of text.
	size_t len;                 // Shows actual number of characters in ptr.
	size_t lim;                 // Shows how many characters can fit in ptr.
	size_t pin;                 // Holds index of the last space in ptr.
	size_t indent;              // Shows how many spaces must be printed in the beginning of ptr.
	struct format_hint **hints;
	size_t *hints_len;
};

bool render_text_html(struct line *line, const struct wstring *source);

bool line_char(struct line *line, wchar_t c);
bool line_string(struct line *line, const wchar_t *str);
bool append_format_hint_to_line(struct line *line, format_hint_mask hint);

void write_contents_of_html_table_node_to_text(struct line *line, GumboNode *node);
#endif // RENDER_DATA_H
