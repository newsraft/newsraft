#include <stdlib.h>
#include "render_data.h"

static inline void
remove_trailing_empty_lines_except_for_first_one(struct line *line)
{
	// Line processor expects there to be at least one line!
	// That's why we always keep the first line.
	for (size_t i = line->target->lines_len - 1; i > 0; --i) {
		for (size_t j = 0; j < line->target->lines[i].ws->len; ++j) {
			if (!ISWIDEWHITESPACE(line->target->lines[i].ws->ptr[j])) {
				return;
			}
		}
		free_wstring(line->target->lines[i].ws);
		free(line->target->lines[i].hints);
		line->target->lines_len -= 1;
		line->head = line->target->lines + line->target->lines_len - 1;
	}
}

bool
render_data(struct render_result *result, struct render_blocks_list *blocks, size_t content_width)
{
	size_t pager_width = get_cfg_uint(CFG_PAGER_WIDTH);
	struct line line = {.target = result};
	line.lim = pager_width > 0 && pager_width < content_width ? pager_width : content_width;
	line_char(&line, L'\n'); // Add first line to line processor
	for (size_t i = 0; i < blocks->len; ++i) {
		line.next_indent = 0;
		if (blocks->ptr[i].content_type == TEXT_HTML) {
			render_text_html(&line, blocks->ptr[i].content);
		} else { // TEXT_RAW || TEXT_PLAIN
			line_string(&line, blocks->ptr[i].content->ptr);
		}
		if (blocks->ptr[i].needs_trimming == true) {
			remove_trailing_empty_lines_except_for_first_one(&line);
		}
		line_style(&line, FORMAT_ALL_END);
	}
	remove_trailing_empty_lines_except_for_first_one(&line);
	if (get_cfg_bool(CFG_PAGER_CENTERING) && pager_width > 0 && pager_width < content_width) {
		for (size_t i = 0; i < result->lines_len; ++i) {
			result->lines[i].indent += (content_width - pager_width) / 2;
		}
	}
	return result->lines_len > 1 ? true : false;
}
