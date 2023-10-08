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
	line->target->lines_len += 1;
	line->pin = SIZE_MAX;
	if (line->is_bold       == true) line_style(line, FORMAT_BOLD_BEGIN);
	if (line->is_underlined == true) line_style(line, FORMAT_UNDERLINED_BEGIN);
	if (line->is_italic     == true) line_style(line, FORMAT_ITALIC_BEGIN);
	return true;
}

bool
line_char(struct line *line, wchar_t c)
{
	if (c == L'\n') {
		return line_bump(line);
	}
	if (LAST_LINE.ws->len == 0) {
		size_t indent_size = line->indent + 1 < line->lim ? line->indent : line->lim - 2;
		for (size_t i = 0; i < indent_size; ++i) {
			wcatcs(LAST_LINE.ws, L' ');
		}
	}
	if (LAST_LINE.ws->len + 1 < line->lim) {
		wcatcs(LAST_LINE.ws, c);
		if (c == L' ') {
			line->pin = LAST_LINE.ws->len - 1;
		}
	} else if (c == L' ') {
		line_bump(line);
	} else if (line->pin == SIZE_MAX) {
		line_bump(line);
		line_char(line, c);
	} else {
		wcatcs(LAST_LINE.ws, c);
		struct wstring *cut = wcrtas(LAST_LINE.ws->ptr + line->pin + 1, wcslen(LAST_LINE.ws->ptr + line->pin + 1));
		LAST_LINE.ws->ptr[line->pin] = L'\0';
		LAST_LINE.ws->len = line->pin;
		line_bump(line);
		line_string(line, cut->ptr);
		free_wstring(cut);
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
	for (size_t i = 0; i < LAST_LINE.hints_len; ++i) {
		if (LAST_LINE.ws->len == LAST_LINE.hints[i].pos) {
			LAST_LINE.hints[i].value |= hint;
			return true;
		}
	}
	void *tmp = realloc(LAST_LINE.hints, sizeof(struct format_hint) * (LAST_LINE.hints_len + 1));
	if (tmp == NULL) {
		return false;
	}
	LAST_LINE.hints = tmp;
	LAST_LINE.hints[LAST_LINE.hints_len].value = hint;
	LAST_LINE.hints[LAST_LINE.hints_len].pos = LAST_LINE.ws->len;
	LAST_LINE.hints_len += 1;
	return true;
}
