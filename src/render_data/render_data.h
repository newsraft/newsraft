#ifndef RENDER_DATA_H
#define RENDER_DATA_H
#include "feedeater.h"

struct line {
	wchar_t *ptr;
	size_t len;
	size_t lim;
	size_t pin;
};

struct wstring *render_text_plain(const struct wstring *wstr);
struct wstring *render_text_html(const struct wstring *wstr);

struct line *create_line(void);
void free_line(struct line *line);
int line_char(struct line *line, wchar_t c, struct wstring *target);
int line_finish(struct line *line, struct wstring *target);
#endif // RENDER_DATA_H
