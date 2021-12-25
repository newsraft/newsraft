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

struct wstring *render_text_plain(const struct wstring *wstr);
struct wstring *render_text_html(const struct wstring *wstr);

struct line *create_line(void);
void free_line(struct line *line);
int line_char(struct line *line, wchar_t c, struct wstring *target);
int line_string(struct line *line, struct wstring *target, wchar_t *str, size_t str_len);
#endif // RENDER_DATA_H
