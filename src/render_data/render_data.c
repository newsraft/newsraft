#include "render_data.h"

struct wstring *
render_data(struct render_blocks_list *blocks)
{
	struct wstring *text = wcrtes(1000);
	if (text == NULL) {
		FAIL("Not enough memory to render data!");
		return NULL;
	}
	struct line line;
	line.target = text;
	line.len = 0;
	line.lim = list_menu_width + 1;
	line.pin = SIZE_MAX;
	line.hints = &blocks->hints;
	line.hints_len = &blocks->hints_len;
	for (size_t i = 0; i < blocks->len; ++i) {
		line.indent = 0;
		if (blocks->ptr[i].content_type == TEXT_HTML) {
			render_text_html(&line, blocks->ptr[i].content);
			append_format_hint_to_line(&line, FORMAT_ALL_END);
		} else { // TEXT_RAW || TEXT_PLAIN
			line_string(&line, blocks->ptr[i].content->ptr);
		}
		trim_whitespace_from_wstring(text);
		for (size_t j = 0; j < blocks->ptr[i].separators_count; ++j) {
			line_char(&line, L'\n');
		}
	}
	trim_whitespace_from_wstring(text);
	// We still need last newline to count in last line of text.
	wcatcs(text, L'\n');
	return text;
}
