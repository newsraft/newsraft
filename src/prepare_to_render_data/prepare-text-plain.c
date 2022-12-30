#include <wctype.h>
#include "prepare_to_render_data/prepare_to_render_data.h"

struct wstring *
prepare_to_render_text_plain(const struct wstring *wide_src, struct links_list *links)
{
	struct wstring *output = wcrtes(wide_src->len + 100);
	if (output == NULL) {
		return NULL;
	}
	int64_t link_start = -1;
	const wchar_t *protocol_delimiter;
	int64_t url_index;
	wchar_t url_mark[100];
	size_t url_mark_len;
	struct string *link_str;
	for (const wchar_t *iter = wide_src->ptr; true; ++iter) {
		if (link_start < 0) {
			if (!ISWIDEWHITESPACE(*iter)) {
				link_start = output->len;
			}
		} else if (ISWIDEWHITESPACE(*iter) || (*iter == L'\0')) {
			protocol_delimiter = wcsstr(output->ptr + link_start, L"://");
			if ((protocol_delimiter != NULL)
				&& (iswalpha(output->ptr[link_start]) != 0)
				&& (wcschr(protocol_delimiter + 3, L'.') != NULL))
			{
				link_str = convert_warray_to_string(output->ptr + link_start, output->len - link_start);
				if (link_str != NULL) {
					url_index = add_another_url_to_trim_links_list(links, link_str->ptr, link_str->len);
					url_mark_len = swprintf(url_mark, 100, L" [%" PRId64 "]", url_index + 1);
					if (url_mark_len > 0) {
						wcatas(output, url_mark, url_mark_len);
					}
					free_string(link_str);
				}
			}
			link_start = -1;
		}
		if (*iter == L'\0') {
			break;
		}
		wcatcs(output, *iter);
	}
	return output;
}
