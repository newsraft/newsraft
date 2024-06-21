#include <stdlib.h>
#include "render_data.h"

bool
line_bump(struct line *line)
{
	struct wstring *line_ws = wcrtes(line->lim);
	if (line_ws == NULL) {
		return false;
	}
	struct render_line *tmp = realloc(line->target->lines, sizeof(struct render_line) * (line->target->lines_len + 1));
	if (tmp == NULL) {
		free_wstring(line_ws);
		return false;
	}
	line->target->lines = tmp;
	line->target->lines_len += 1;
	line->head = line->target->lines + line->target->lines_len - 1;
	line->head->ws = line_ws;
	line->head->hints = NULL;
	line->head->hints_len = 0;
	line->head->indent = line->next_indent;
	line->end = SIZE_MAX;
	return true;
}

static inline void
line_split_at_end(struct line *line)
{
	// Have to use pointers to individual members because calling line_char can
	// realloc lines array which would make using a pointer to the render_line
	// element prone to the use-after-free bugs.
	struct wstring *prev_ws = line->head->ws;
	format_mask *prev_hints = line->head->hints;
	size_t prev_end         = line->end;

	line_bump(line); // Now line->head points to a new empty line

	size_t i = prev_end + 1;
	while (i < prev_ws->len && ISWIDEWHITESPACE(prev_ws->ptr[i])) {
		i += 1;
	}
	while (i < prev_ws->len) {
		line->style = prev_hints[i];
		line_char(line, prev_ws->ptr[i++]);
	}

	prev_ws->ptr[prev_end + 1] = L'\0';
	prev_ws->len = prev_end + 1;
}

// This function writes a character and its style to a render target.
// Writing to a render target must be done only through this function!
static bool
line_append(struct line *line, wchar_t c)
{
	wcatcs(line->head->ws, c);

	if (line->head->hints_len < line->head->ws->lim) {
		line->head->hints = realloc(line->head->hints, sizeof(format_mask) * (line->head->ws->lim + 1));
		line->head->hints_len = line->head->ws->lim;
	}

	line->head->hints[line->head->ws->len - 1] = line->style;

	return true;
}

bool
line_char(struct line *line, wchar_t c)
{
	if (c == L'\n') return line_bump(line);
	if (line->lim == 0) return line_append(line, c);

	int c_width = wcwidth(c);
	if (c_width < 1) return true; // Ignore invalid characters.

	if (c == L' '
		&& line->head->ws->len > 0
		&& line->head->ws->ptr[line->head->ws->len - 1] != ' ')
	{
		line->end = line->head->ws->len - 1;
	}

	if ((size_t)wcswidth(line->head->ws->ptr, line->head->ws->len) + c_width <= line->lim - line->head->indent) {
		line_append(line, c);
	} else if (c == L' ') {
		return true; // Ignore spaces when we are in the end of line
	} else if (line->end == SIZE_MAX) {
		line_bump(line);
		line_char(line, c);
	} else {
		line_append(line, c);
		line_split_at_end(line);
	}

	return true; // TODO: check for errors?
}

bool
line_string(struct line *line, const wchar_t *str)
{
	for (const wchar_t *i = str; *i != L'\0'; ++i) {
		line_char(line, *i);
	}
	return true; // TODO: check for errors?
}
