#include <stdlib.h>
#include "render_data.h"

bool
line_char(struct line *line, wchar_t c)
{
	if (c == L'\n') {
		wcatas(line->target, line->ptr, line->len);
		wcatcs(line->target, L'\n');
		line->len = 0;
		line->pin = SIZE_MAX;
		return true;
	}
	if (line->len == 0) {
		for (size_t i = 0; (i < line->indent) && (i < line->lim); ++i) {
			line->ptr[i] = L' ';
		}
		line->len = MIN(line->indent, line->lim);
	}
	line->ptr[line->len] = c;
	if (c == L' ') {
		line->pin = line->len;
	}
	line->len += 1;
	if (line->len != line->lim) {
		return true;
	}
	if (line->ptr[line->len - 1] == L' ') {
		wcatas(line->target, line->ptr, line->len);
		line->len = 0;
		line->pin = SIZE_MAX;
	} else if (line->pin == SIZE_MAX) {
		wcatas(line->target, line->ptr, line->len);
		line->ptr[0] = line->ptr[line->len - 1];
		line->len = 1;
	} else {
		wcatas(line->target, line->ptr, line->pin);
		for (size_t i = 0; (i < line->indent) && (i < line->lim); ++i) {
			line->ptr[i] = L' ';
		}
		size_t new_len = MIN(line->indent, line->lim);
		for (size_t i = line->pin + 1; i < line->len; ++i) {
			line->ptr[new_len++] = line->ptr[i];
		}
		line->len = new_len;
		line->pin = SIZE_MAX;
	}
	wcatcs(line->target, L'\n');
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
	size_t position = line->target->len + line->len;
	for (size_t i = 0; i < *line->hints_len; ++i) {
		if (position == (*line->hints)[i].pos) {
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
	(*line->hints)[*line->hints_len].pos = position;
	*line->hints_len += 1;
	return true;
}
