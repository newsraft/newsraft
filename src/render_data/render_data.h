#ifndef RENDER_DATA_H
#define RENDER_DATA_H
#include <gumbo.h>
#include "newsraft.h"

struct line {
	struct render_result *target; // Render output context
	struct render_line *head;     // Line where text is currently added
	size_t lim;                   // Capacity of one text line
	size_t end;                   // Index of character suitable for line ending
	size_t indent;                // Indentation for subsequent line bumps

	newsraft_video_t style;        // Cumulative style value for added text
	newsraft_video_t *style_stack; // Stack of all currently applied styles
	size_t style_stack_len;
};

bool render_text_plain(struct line *line, const struct wstring *source, struct links_list *links);
bool render_text_html(struct line *line, const struct wstring *source, struct links_list *links);

bool line_char(struct line *line, wchar_t c);
bool line_string(struct line *line, const wchar_t *str);
void line_style(struct line *line, newsraft_video_t attrs);
void line_unstyle(struct line *line);
#endif // RENDER_DATA_H
