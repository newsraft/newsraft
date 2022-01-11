#ifndef RENDER_DATA_H
#define RENDER_DATA_H
#include "feedeater.h"

struct line {
	wchar_t *ptr;  // Actual text string that represents line.
	size_t len;    // Shows number of characters in line.
	size_t lim;    // Shows how many characters can fit in line.
	size_t pin;    // Holds index of last break character in line.
	size_t indent; // Shows how many spaces must be printed in the beginning of line.
};

bool render_text_plain(const struct wstring *source, struct line *line, struct wstring *target);
bool render_text_html(const struct wstring *source, struct line *line, struct wstring *target);

bool line_char(struct line *line, wchar_t c, struct wstring *target);
bool line_string(struct line *line, const wchar_t *str, struct wstring *target);
#endif // RENDER_DATA_H
