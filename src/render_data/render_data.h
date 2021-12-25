#ifndef RENDER_DATA_H
#define RENDER_DATA_H
#include "feedeater.h"

// This has to be the length of the longest HTML entity name in entities array.
#define MAX_ENTITY_NAME_LENGTH 13

struct line {
	wchar_t *ptr;  // Actual text string that represents line.
	size_t len;    // Shows number of characters in line.
	size_t lim;    // Shows how many characters can fit in line.
	size_t pin;    // Holds index of last break character in line.
	size_t indent; // Shows how many spaces must be printed in the beginning of line.
};

struct wstring *render_text_plain(const struct wstring *wstr);
struct wstring *render_text_html(const struct wstring *wstr);
const wchar_t *translate_html_entity(wchar_t *entity);

struct line *create_line(void);
void free_line(struct line *line);
int line_char(struct line *line, wchar_t c, struct wstring *target);
int line_string(struct line *line, struct wstring *target, const wchar_t *str);
#endif // RENDER_DATA_H
