#include <stdlib.h>
#include "render_data.h"

struct line *
create_line(void)
{
	struct line *line = malloc(sizeof(struct line));
	if (line == NULL) {
		return NULL;
	}
	line->ptr = malloc(sizeof(wchar_t) * (list_menu_width + 1));
	if (line->ptr == NULL) {
		free(line);
		return NULL;
	}
	line->len = 0;
	line->lim = list_menu_width;
	line->pin = SIZE_MAX;
	line->indent = 0;
	return line;
}

void
free_line(struct line *line)
{
	if (line == NULL) {
		return;
	}
	free(line->ptr);
	free(line);
}

int
line_char(struct line *line, wchar_t c, struct wstring *target)
{
	if (c == L'\n') {
		wcatas(target, line->ptr, line->len);
		wcatcs(target, L'\n');
		line->len = 0;
		line->pin = SIZE_MAX;
		return 0;
	}
	if (line->len == 0) {
		for (size_t i = 0; i < line->indent; ++i) {
			line->ptr[i] = L' ';
		}
		line->len = line->indent;
	}
	line->ptr[line->len] = c;
	if (is_wchar_a_breaker(c) == true) {
		line->pin = line->len;
	}
	++(line->len);
	if (line->len != line->lim) {
		return 0;
	}
	if (line->pin == SIZE_MAX) {
		wcatas(target, line->ptr, line->len);
		line->len = 0;
	} else {
		wcatas(target, line->ptr, line->pin + 1);
		for (size_t i = 0; i < line->indent; ++i) {
			line->ptr[i] = L' ';
		}
		size_t new_len = line->indent;
		for (size_t i = line->pin + 1; i < line->len; ++i) {
			line->ptr[new_len++] = line->ptr[i];
		}
		line->len = new_len;
		line->pin = SIZE_MAX;
	}
	wcatcs(target, L'\n');
	return 0;
}

int
line_string(struct line *line, struct wstring *target, const wchar_t *str)
{
	const wchar_t *iter = str;
	while (*iter != L'\0') {
		line_char(line, *iter, target);
		++iter;
	}
	return 0;
}
