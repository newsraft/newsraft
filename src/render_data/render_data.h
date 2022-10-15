#ifndef RENDER_DATA_H
#define RENDER_DATA_H
#include <gumbo.h>
#include "newsraft.h"

struct line {
	wchar_t *ptr;  // Line text.
	size_t len;    // Shows actual number of characters in ptr.
	size_t lim;    // Shows how many characters can fit in ptr.
	size_t pin;    // Holds index of the last space in ptr.
	size_t indent; // Shows how many spaces must be printed in the beginning of ptr.
};

bool render_text_html(const struct wstring *source, struct line *line, struct wstring *target);

bool line_char(struct line *line, wchar_t c, struct wstring *target);
bool line_string(struct line *line, const wchar_t *str, struct wstring *target);

void write_contents_of_html_table_node_to_text(struct wstring *text, struct line *line, GumboNode *node);
#endif // RENDER_DATA_H
