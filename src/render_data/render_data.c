#include <stdlib.h>
#include "render_data.h"

struct wstring *
render_data(const struct render_blocks_list *blocks)
{
	struct wstring *text = wcrtes();
	if (text == NULL) {
		FAIL("Not enough memory to render data!");
		return NULL;
	}
	struct line line;
	line.ptr = malloc(sizeof(wchar_t) * (list_menu_width + 1));
	if (line.ptr == NULL) {
		FAIL("Not enough memory for line buffer to render data!");
		free_wstring(text);
		return NULL;
	}
	line.len = 0;
	line.lim = list_menu_width + 1;
	line.pin = SIZE_MAX;
	for (size_t i = 0; i < blocks->len; ++i) {
		line.indent = 0;
		if (blocks->ptr[i].content_type == TEXT_PLAIN) {
			line_string(&line, blocks->ptr[i].content->ptr, text);
			if (line.len == 0) {
				trim_whitespace_from_wstring(text);
			}
		} else if (blocks->ptr[i].content_type == TEXT_HTML) {
			render_text_html(blocks->ptr[i].content, &line, text);
			if (line.len == 0) {
				trim_whitespace_from_wstring(text);
			}
		} else if (blocks->ptr[i].content_type == TEXT_SEPARATOR) {
			line_char(&line, L'\n', text);
		}
	}
	for (size_t i = 0; i < line.len; ++i) {
		wcatcs(text, line.ptr[i]);
	}
	free(line.ptr);
	trim_whitespace_from_wstring(text);
	// We still need last newline to count in last line of text.
	wcatcs(text, L'\n');
	return text;
}
