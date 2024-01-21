#include <stdlib.h>
#include "render_data.h"

bool
line_bump(struct line *line)
{
	struct wstring *line_ws = wcrtes(50);
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
	line->pin = SIZE_MAX;

	// Apply unfinished formatting of the previous line to the current line.
	// We do it in line_bump instead of line_char to make sure formatting may
	// continue on empty lines (where line_char is not called).
	if (line->target->lines_len > 1 && (line->head - 1)->hints_len > 0) {
		for (size_t i = 0; i < (line->head - 1)->hints_len; ++i) {
			line_style(line, (line->head - 1)->hints[i].mask);
		}
		line->head->hints[0].mask &= ~FORMAT_ALL_END;
	}

	return true;
}

static inline void
line_pin_split(struct line *line)
{
	size_t prev_pin = line->pin;
	size_t prev_hints_len = line->head->hints_len;
	size_t next_hints_start = 0;
	while (next_hints_start < prev_hints_len) {
		// Strict inequality matters in cases where text
		// formatting ends right before the pin character.
		// For example, "<u>Lorem ipsum</u>{pin}".
		if (line->head->hints[next_hints_start].pos > prev_pin) {
			line->head->hints_len = next_hints_start;
			break;
		}
		next_hints_start += 1;
	}

	line_bump(line); // Now line->head points to a next empty line.

	struct render_line *prev_head = line->head - 1;
	size_t prev_len = prev_head->ws->len;
	prev_head->ws->ptr[prev_pin] = L'\0';
	prev_head->ws->len = prev_pin;

	for (size_t i = prev_pin + 1, j = next_hints_start; i < prev_len; ++i) {
		if (j < prev_hints_len && i == prev_head->hints[j].pos) {
			line_style(line, prev_head->hints[j].mask);
			j += 1;
		}
		line_char(line, prev_head->ws->ptr[i]);
	}
}

bool
line_char(struct line *line, wchar_t c)
{
	if (c == L'\n') return line_bump(line);

	int c_width = wcwidth(c);
	if (c_width < 1) return true; // Ignore invalid characters.

	if ((size_t)wcswidth(line->head->ws->ptr, line->head->ws->len) + c_width <= line->lim - line->head->indent) {
		wcatcs(line->head->ws, c);
		if (c == L' ') {
			line->pin = line->head->ws->len - 1;
		}
	} else if (c == L' ') {
		line_bump(line);
	} else if (line->pin == SIZE_MAX) {
		line_bump(line);
		line_char(line, c);
	} else {
		wcatcs(line->head->ws, c);
		line_pin_split(line);
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

bool
line_style(struct line *line, format_hint_mask hint)
{
	for (size_t i = 0; i < line->head->hints_len; ++i) {
		if (line->head->ws->len == line->head->hints[i].pos) {
			line->head->hints[i].mask |= hint;
			if (hint & FORMAT_BOLD_BEGIN)       line->head->hints[i].mask &= ~FORMAT_BOLD_END;
			if (hint & FORMAT_BOLD_END)         line->head->hints[i].mask &= ~FORMAT_BOLD_BEGIN;
			if (hint & FORMAT_UNDERLINED_BEGIN) line->head->hints[i].mask &= ~FORMAT_UNDERLINED_END;
			if (hint & FORMAT_UNDERLINED_END)   line->head->hints[i].mask &= ~FORMAT_UNDERLINED_BEGIN;
			if (hint & FORMAT_ITALIC_BEGIN)     line->head->hints[i].mask &= ~FORMAT_ITALIC_END;
			if (hint & FORMAT_ITALIC_END)       line->head->hints[i].mask &= ~FORMAT_ITALIC_BEGIN;
			return true;
		}
	}
	void *tmp = realloc(line->head->hints, sizeof(struct format_hint) * (line->head->hints_len + 1));
	if (tmp == NULL) {
		return false;
	}
	line->head->hints = tmp;
	line->head->hints[line->head->hints_len].mask = hint;
	line->head->hints[line->head->hints_len].pos = line->head->ws->len;
	line->head->hints_len += 1;
	return true;
}
