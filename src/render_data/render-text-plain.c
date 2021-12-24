#include <string.h>
#include "render_data.h"

struct wstring *
render_text_plain(const struct wstring *wstr)
{
	struct wstring *result = create_empty_wstring();
	if (result == NULL) {
		return NULL;
	}
	wchar_t *line = malloc(sizeof(wchar_t) * (list_menu_width + 1));
	if (line == NULL) {
		free_wstring(result);
		return NULL;
	}
	ssize_t line_len = 0;
	ssize_t new_line_len;

	const wchar_t *iter = wstr->ptr;
	size_t last_breaker_index_in_line = SIZE_MAX;

	while (*iter != L'\0') {
		if (line_len == list_menu_width) {
			line[line_len] = L'\0';

			if (last_breaker_index_in_line == SIZE_MAX) {
				wcatas(result, line, line_len);
				line_len = 0;
			} else {
				wcatas(result, line, last_breaker_index_in_line + 1);
				new_line_len = 0;
				for (ssize_t i = last_breaker_index_in_line + 1; i < line_len; ++i) {
					line[new_line_len++] = line[i];
				}
				line[new_line_len] = L'\0';
				line_len = new_line_len;
			}

			wcatcs(result, L'\n');

			last_breaker_index_in_line = SIZE_MAX;
		} else if (*iter == L'\n') {
			wcatas(result, line, line_len);
			wcatcs(result, L'\n');
			line_len = 0;
			last_breaker_index_in_line = SIZE_MAX;
			++iter;
		} else {
			line[line_len++] = *iter;
			if (is_wchar_a_breaker(*iter)) {
				last_breaker_index_in_line = line_len - 1;
			}
			++iter;
		}
	}

	if (line_len != 0) {
		wcatas(result, line, line_len);
		wcatcs(result, L'\n');
	}

	free(line);

	return result;
}
