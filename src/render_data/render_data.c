#include <stdlib.h>
#include "render_data.h"

static inline void
remove_trailing_empty_lines_except_for_first_one(struct render_result *r)
{
	// Line processor expects there to be at least one line!
	// That's why we always keep the first line.
	for (size_t i = r->lines_len - 1; i > 0; --i) {
		for (size_t j = 0; j < r->lines[i].ws->len; ++j) {
			if (!ISWIDEWHITESPACE(r->lines[i].ws->ptr[j])) {
				return;
			}
		}
		free_wstring(r->lines[i].ws);
		free(r->lines[i].hints);
		r->lines_len -= 1;
	}
}

bool
render_data(struct render_result *result, struct render_blocks_list *blocks)
{
	struct line line = {.target = result, .lim = list_menu_width};
	line_bump(&line); // Add first line to line processor.
	for (size_t i = 0; i < blocks->len; ++i) {
		line.pin = SIZE_MAX;
		line.indent = 0;
		if (blocks->ptr[i].content_type == TEXT_HTML) {
			render_text_html(&line, blocks->ptr[i].content);
		} else { // TEXT_RAW || TEXT_PLAIN
			line_string(&line, blocks->ptr[i].content->ptr);
		}
		if (blocks->ptr[i].needs_trimming == true) {
			remove_trailing_empty_lines_except_for_first_one(result);
		}
		line_style(&line, FORMAT_ALL_END);
	}
	remove_trailing_empty_lines_except_for_first_one(result);
	return result->lines_len > 1 ? true : false;
}
