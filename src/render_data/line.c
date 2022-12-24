#include <stdlib.h>
#include "render_data.h"

bool
line_char(struct line *line, wchar_t c)
{
	if (c == L'\n') {
		wcatcs(line->target, L'\n');
		line->len = 0;
		line->pin = SIZE_MAX;
		return true;
	}
	if (line->len == 0) {
		line->len = line->indent + 1 < line->lim ? line->indent : line->lim - 2;
		for (size_t i = 0; i < line->len; ++i) {
			wcatcs(line->target, L' ');
		}
	}
	if (line->len + 1 < line->lim) {
		wcatcs(line->target, c);
		line->len += 1;
		if (c == L' ') {
			line->pin = line->target->len - 1;
		}
	} else if (c == L' ') {
		wcatcs(line->target, L'\n');
		line->len = 0;
		line->pin = SIZE_MAX;
	} else if (line->pin == SIZE_MAX) {
		wcatcs(line->target, L'\n');
		wcatcs(line->target, c);
		line->len = 1;
	} else {
		wcatcs(line->target, c);
		line->target->ptr[line->pin] = L'\n';
		struct wstring *cut = wcrtas(line->target->ptr + line->pin + 1, wcslen(line->target->ptr + line->pin + 1));
		line->target->len = line->pin + 1;
		line->len = line->indent + 1 < line->lim ? line->indent : line->lim - 2;
		for (size_t i = 0; i < line->len; ++i) {
			wcatcs(line->target, L' ');
		}
		wcatss(line->target, cut);
		line->len += cut->len;
		line->pin = SIZE_MAX;
		free_wstring(cut);
	}
	return true; // TODO: check for errors?
}

bool
line_string(struct line *line, const wchar_t *str)
{
	while (*str != L'\0') {
		line_char(line, *str++);
	}
	return true; // TODO: check for errors?
}

bool
append_format_hint_to_line(struct line *line, format_hint_mask hint)
{
	for (size_t i = 0; i < *line->hints_len; ++i) {
		if (line->target->len == (*line->hints)[i].pos) {
			(*line->hints)[i].value |= hint;
			return true;
		}
	}
	void *tmp = realloc(*line->hints, sizeof(struct format_hint) * (*line->hints_len + 1));
	if (tmp == NULL) {
		return false;
	}
	*line->hints = tmp;
	(*line->hints)[*line->hints_len].value = hint;
	(*line->hints)[*line->hints_len].pos = line->target->len;
	*line->hints_len += 1;
	return true;
}
