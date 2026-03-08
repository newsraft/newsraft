#include <stdlib.h>
#include "render_data.h"

bool
render_text_plain(struct line *line, const struct wstring *source, struct links_list *links)
{
	for (const wchar_t *i = source->ptr, *j = i; *i != L'\0'; i = j) {

		// find end of token
		while (!ISWIDEWHITESPACE(*j) && *j != L'\0') {
			j += 1;
		}

		// render token characters
		for (const wchar_t *k = i; k < j; ++k) {
			line_char(line, *k);
		}

		// render link mark if token is URI
		const wchar_t *scheme_delimiter = wcsstr(i, L"://");
		if (scheme_delimiter && scheme_delimiter > i && scheme_delimiter < j) {
			// token contains scheme delimiter, so it's a good candidate for URI
			const wchar_t *uri_start = i;

			// RFC 3986 says URI scheme can only begin with ASCII letter
			while (uri_start < scheme_delimiter
				&& (*uri_start < L'A' || *uri_start > L'Z')
				&& (*uri_start < L'a' || *uri_start > L'z'))
			{
				uri_start += 1;
			}

			if (uri_start < scheme_delimiter) {
				struct string *link = convert_warray_to_string(uri_start, j - uri_start);
				if (link && link->len) {
					wchar_t url_mark[100];
					size_t url_index = add_url_to_links_list(links, link->ptr, link->len);
					// U+00A0 is non-breaking space
					if (swprintf(url_mark, 100, L"\u00A0[%zu]", url_index + 1) > 0) {
						line_style(line, TB_BOLD);
						line_string(line, url_mark);
						line_unstyle(line);
					}
				}
				free_string(link);
			}
		}

		// spew out whitespace character which concluded current token
		if (*j != L'\0') {
			line_char(line, *j);
			j += 1;
		}

	}

	return true;
}
