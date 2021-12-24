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
	if (line->len == line->lim) {
		size_t new_len = 0;
		if (line->pin == SIZE_MAX) {
			wcatas(target, line->ptr, line->len);
		} else {
			wcatas(target, line->ptr, line->pin + 1);
			for (size_t i = line->pin + 1; i < line->len; ++i) {
				if ((new_len == 0) && ((line->ptr[i] == L' ') || (line->ptr[i] == L'\t'))) {
					continue; // Skip leading whitespace.
				}
				line->ptr[new_len++] = line->ptr[i];
			}
			line->pin = SIZE_MAX;
		}
		line->len = new_len;
		wcatcs(target, L'\n');
	}
	line->ptr[line->len] = c;
	if (is_wchar_a_breaker(c) == true) {
		line->pin = line->len;
	}
	++(line->len);
	return 0;
}

int
line_finish(struct line *line, struct wstring *target)
{
	if (line->len != 0) {
		wcatas(target, line->ptr, line->len);
		wcatcs(target, L'\n');
	}
	return 0;
}
