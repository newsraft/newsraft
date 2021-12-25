#include "render_data.h"

struct wstring *
render_text_plain(const struct wstring *wstr)
{
	struct wstring *result = create_empty_wstring();
	if (result == NULL) {
		return NULL;
	}
	struct line *line = create_line();
	if (line == NULL) {
		free_wstring(result);
		return NULL;
	}
	const wchar_t *iter = wstr->ptr;
	while (*iter != L'\0') {
		line_char(line, *iter, result);
		++iter;
	}
	if (line->len != 0) {
		line_char(line, L'\n', result);
	}
	free_line(line);
	return result;
}
