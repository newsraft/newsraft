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
	tmp[line->target->lines_len].ws = line_ws;
	tmp[line->target->lines_len].hints = NULL;
	tmp[line->target->lines_len].hints_len = 0;
	line->head = tmp + line->target->lines_len;
	line->target->lines = tmp;
	line->target->lines_len += 1;
	line->pin = SIZE_MAX;
	return true;
}

static inline void
line_pin_split(struct line *line)
{
	size_t prev_pin = line->pin;
	line_bump(line); // Now line->head points to a next empty line.
	struct render_line *prev_head = line->head - 1;
	prev_head->ws->ptr[prev_pin] = L'\0';
	prev_head->ws->len = prev_pin;

	size_t next_hints_start;
	for (next_hints_start = 0; next_hints_start < prev_head->hints_len; ++next_hints_start) {
		if (prev_head->hints[next_hints_start].pos > prev_pin) {
			break;
		}
	}

	if (next_hints_start < prev_head->hints_len) {
		// Trim content and style hints of current line head.
		for (size_t i = 0; i < next_hints_start; ++i) {
			line_style(line, prev_head->hints[i].mask);
		}
		for (size_t i = next_hints_start; i < prev_head->hints_len; ++i) {
			struct format_hint *tmp = realloc(line->head->hints, sizeof(struct format_hint) * (line->head->hints_len + 1));
			if (tmp == NULL) return;
			line->head->hints = tmp;
			line->head->hints[line->head->hints_len].mask = prev_head->hints[i].mask;
			line->head->hints[line->head->hints_len].pos = prev_head->hints[i].pos - prev_pin - 1;
			line->head->hints_len += 1;
		}
		prev_head->hints_len = next_hints_start;
	}

	line_string(line, prev_head->ws->ptr + prev_pin + 1);
}

bool
line_char(struct line *line, wchar_t c)
{
	if (c == L'\n') {
		return line_bump(line);
	}
	const int c_width = wcwidth(c);
	if (c_width < 1) {
		return true; // Ignore invalid characters.
	}
	if (line->head->ws->len == 0) {
		size_t indent_size = line->indent < line->lim ? line->indent : line->lim - 1;
		for (size_t i = 0; i < indent_size; ++i) {
			wcatcs(line->head->ws, L' ');
		}
		// Apply unfinished formatting of the previous line to the current line.
		// We do it AFTER indenting whitespace to avoid styling empty start of line.
		if (line->target->lines_len > 1) {
			bool is_bold = false, is_underlined = false, is_italic = false;
			for (size_t i = 0; i < (line->head - 1)->hints_len; ++i) {
				if ((line->head - 1)->hints[i].mask & FORMAT_BOLD_BEGIN)       is_bold = true;
				if ((line->head - 1)->hints[i].mask & FORMAT_BOLD_END)         is_bold = false;
				if ((line->head - 1)->hints[i].mask & FORMAT_UNDERLINED_BEGIN) is_underlined = true;
				if ((line->head - 1)->hints[i].mask & FORMAT_UNDERLINED_END)   is_underlined = false;
				if ((line->head - 1)->hints[i].mask & FORMAT_ITALIC_BEGIN)     is_italic = true;
				if ((line->head - 1)->hints[i].mask & FORMAT_ITALIC_END)       is_italic = false;
			}
			if (is_bold       == true) line_style(line, FORMAT_BOLD_BEGIN);
			if (is_underlined == true) line_style(line, FORMAT_UNDERLINED_BEGIN);
			if (is_italic     == true) line_style(line, FORMAT_ITALIC_BEGIN);
		}
	}
	if (wcswidth(line->head->ws->ptr, line->head->ws->len) + c_width <= (int)line->lim) {
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
