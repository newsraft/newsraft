#include "render_data/render_data.h"

bool
render_text_plain(const struct wstring *source, struct line *line, struct wstring *target)
{
	return line_string(line, source->ptr, target);
}
