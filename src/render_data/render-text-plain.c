#include "render_data/render_data.h"

bool
render_text_plain(const struct wstring *source, struct line *line, struct wstring *target, bool is_first_call)
{
	(void)is_first_call;
	return line_string(line, source->ptr, target);
}
