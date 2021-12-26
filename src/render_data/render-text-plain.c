#include "render_data.h"

struct wstring *
render_text_plain(const struct wstring *wstr, struct line *line)
{
	struct wstring *result = create_empty_wstring();
	if (result == NULL) {
		return NULL;
	}
	const wchar_t *iter = wstr->ptr;
	while (*iter != L'\0') {
		line_char(line, *iter, result);
		++iter;
	}
	return result;
}
