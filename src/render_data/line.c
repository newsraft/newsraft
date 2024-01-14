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
	line->target->lines = tmp;
	line->head = line->target->lines + line->target->lines_len;
	line->target->lines_len += 1;
	line->pin = SIZE_MAX;
	return true;
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
		// Make sure to apply styling after indenting whitespace to avoid
		// underlined empty space at the beginning of the line, for example.
		if (line->is_bold       == true) line_style(line, FORMAT_BOLD_BEGIN);
		if (line->is_underlined == true) line_style(line, FORMAT_UNDERLINED_BEGIN);
		if (line->is_italic     == true) line_style(line, FORMAT_ITALIC_BEGIN);
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
		const wchar_t *cut = line->head->ws->ptr + line->pin + 1;
		line->head->ws->ptr[line->pin] = L'\0';
		line->head->ws->len = line->pin;
		line_bump(line);
		line_string(line, cut);
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
	if (hint & FORMAT_BOLD_BEGIN)       line->is_bold       = true;
	if (hint & FORMAT_BOLD_END)         line->is_bold       = false;
	if (hint & FORMAT_UNDERLINED_BEGIN) line->is_underlined = true;
	if (hint & FORMAT_UNDERLINED_END)   line->is_underlined = false;
	if (hint & FORMAT_ITALIC_BEGIN)     line->is_italic     = true;
	if (hint & FORMAT_ITALIC_END)       line->is_italic     = false;
	for (size_t i = 0; i < line->head->hints_len; ++i) {
		if (line->head->ws->len == line->head->hints[i].pos) {
			line->head->hints[i].value |= hint;
			// Force style on when explicitly enabling it.
			if (hint & FORMAT_BOLD_BEGIN)       line->head->hints[i].value &= ~FORMAT_BOLD_END;
			if (hint & FORMAT_UNDERLINED_BEGIN) line->head->hints[i].value &= ~FORMAT_UNDERLINED_END;
			if (hint & FORMAT_ITALIC_BEGIN)     line->head->hints[i].value &= ~FORMAT_ITALIC_END;
			return true;
		}
	}
	void *tmp = realloc(line->head->hints, sizeof(struct format_hint) * (line->head->hints_len + 1));
	if (tmp == NULL) {
		return false;
	}
	line->head->hints = tmp;
	line->head->hints[line->head->hints_len].value = hint;
	line->head->hints[line->head->hints_len].pos = line->head->ws->len;
	line->head->hints_len += 1;
	return true;
}
