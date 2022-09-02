#include <stdlib.h>
#include "render_data.h"

bool
line_char(struct line *line, wchar_t c, struct wstring *target)
{
	if (c == L'\n') {
		wcatas(target, line->ptr, line->len);
		wcatcs(target, L'\n');
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
		wcatas(target, line->ptr, line->len);
		line->len = 0;
		line->pin = SIZE_MAX;
	} else if (line->pin == SIZE_MAX) {
		wcatas(target, line->ptr, line->len);
		line->ptr[0] = line->ptr[line->len - 1];
		line->len = 1;
	} else {
		wcatas(target, line->ptr, line->pin);
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
	wcatcs(target, L'\n');
	return true;
}

bool
line_string(struct line *line, const wchar_t *str, struct wstring *target)
{
	const wchar_t *iter = str;
	while (*iter != L'\0') {
		line_char(line, *iter, target);
		++iter;
	}
	return true;
}
