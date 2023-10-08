#include "render_data.h"

static inline void
remove_trailing_empty_lines(struct render_result *r)
{
	for (size_t i = r->lines_len; i > 0; --i) {
		for (size_t j = 0; j < r->lines[i - 1].ws->len; ++j) {
			if (!ISWIDEWHITESPACE(r->lines[i - 1].ws->ptr[j])) {
				return;
			}
		}
		free_wstring(r->lines[i - 1].ws);
		free(r->lines[i - 1].hints);
		r->lines_len -= 1;
	}
}

bool
render_data(struct render_result *result, struct render_blocks_list *blocks)
{
	struct line line = {.target = result, .lim = list_menu_width + 1};
	line_bump(&line); // Add first line to line processor.
	for (size_t i = 0; i < blocks->len; ++i) {
		line.pin = SIZE_MAX;
		line.indent = 0;
		if (blocks->ptr[i].content_type == TEXT_HTML) {
			render_text_html(&line, blocks->ptr[i].content);
		} else { // TEXT_RAW || TEXT_PLAIN
			line_string(&line, blocks->ptr[i].content->ptr);
		}
		remove_trailing_empty_lines(result);
		line_style(&line, FORMAT_ALL_END);
		for (size_t j = 0; j < blocks->ptr[i].separators_count; ++j) {
			line_char(&line, L'\n');
		}
	}
	remove_trailing_empty_lines(result);
	return result->lines_len > 0 ? true : false;
}
