#ifndef RENDER_DATA_H
#define RENDER_DATA_H
#include <gumbo.h>
#include "newsraft.h"

struct line {
	struct render_result *target; // Render output context
	struct render_line *head;     // Line where text is currently added
	size_t lim;                   // Capacity of one text line
	size_t end;                   // Index of character suitable for line ending
	size_t next_indent;           // Indentation for subsequent line bumps
	format_mask style;
};

bool render_text_html(struct line *line, const struct wstring *source);

bool line_char(struct line *line, wchar_t c);
bool line_string(struct line *line, const wchar_t *str);
#endif // RENDER_DATA_H
