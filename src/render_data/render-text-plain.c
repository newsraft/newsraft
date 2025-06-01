#include <stdlib.h>
#include "render_data.h"

bool
render_text_plain(struct line *line, const struct wstring *source, struct links_list *links)
{
	for (const wchar_t *i = source->ptr, *j = NULL; *i != L'\0'; i = j) {
		j = i;
		while (!ISWIDEWHITESPACE(*j) && *j != L'\0') {
			j += 1;
		}
		struct string *link = NULL;
		wchar_t *divider = wcsstr(i, L"://");
		if (divider > i && divider < j) {
			link = convert_warray_to_string(i, j - i);
		}
		for (const wchar_t *k = i; k < j; ++k) {
			line_char(line, *k);
		}
		if (link != NULL && link->len > 0) {
			wchar_t url_mark[100];
			size_t url_index = add_url_to_links_list(links, link->ptr, link->len);
			// Space character before the mark is &nbsp;
			if (swprintf(url_mark, 100, L" [%" PRId64 "]", url_index + 1) > 0) {
				line_style(line, TB_BOLD);
				line_string(line, url_mark);
				line_unstyle(line);
			}
		}
		free_string(link);
		link = NULL;
		if (*j != L'\0') {
			line_char(line, *j);
			j += 1;
		}
	}
	return true;
}
