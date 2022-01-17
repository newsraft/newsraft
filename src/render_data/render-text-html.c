#include <stdlib.h>
#include <string.h>
#include "render_data.h"

#define MAX_NESTED_LISTS_DEPTH 10
#define SPACES_PER_INDENTATION_LEVEL 4
#define SPACES_PER_BLOCKQUOTE_LEVEL 4
#define SPACES_PER_FIGURE_LEVEL 4

enum html_position {
	HTML_NONE = 0,
	HTML_PRE = 1,
};

enum list_type {
	UNORDERED_LIST,
	ORDERED_LIST,
};

struct list_level {
	enum list_type type;
	uint16_t length;
};

enum html_position html_pos;
static uint8_t list_depth;
static struct list_level list_levels[MAX_NESTED_LISTS_DEPTH];

static void
add_newlines(struct line *line, struct wstring *text, uint8_t count)
{
	uint8_t how_many_newlines_already_present = 0;
	if (line->len == 0) {
		for (uint8_t i = 1; i <= count; ++i) {
			if ((text->len >= i) && (text->ptr[text->len - i] == L'\n')) {
				++how_many_newlines_already_present;
			} else {
				break;
			}
		}
	}
	for (uint8_t i = 0; i < count - how_many_newlines_already_present; ++i) {
		line_char(line, L'\n', text);
	}
}

static inline void
br_handler(struct line *line, struct wstring *text)
{
	line_char(line, L'\n', text);
}

static inline void
hr_handler(struct line *line, struct wstring *text)
{
	add_newlines(line, text, 1);
	for (size_t i = 0; i < line->lim; ++i) {
		line_char(line, L'─', text);
	}
	line_char(line, L'\n', text);
}

static inline void
li_start_handler(struct line *line, struct wstring *text)
{
	add_newlines(line, text, 1);
	line->indent = list_depth * SPACES_PER_INDENTATION_LEVEL;
	if (list_levels[list_depth - 1].type == UNORDERED_LIST) {
		line_string(line, L"*  ", text);;
		line->indent += 3;
	} else {
		++(list_levels[list_depth - 1].length);
		// 9 = 5 (for longest uint16_t) + 2 (for dot and space) + 1 (for terminator) + 1 (for luck lol)
		wchar_t number_str[9];
		size_t number_str_len = swprintf(number_str, 9, L"%d. ", list_levels[list_depth - 1].length);
		line_string(line, number_str, text);
		line->indent += number_str_len;
	}
}

static inline void
ul_start_handler(struct line *line, struct wstring *text)
{
	if (list_depth == MAX_NESTED_LISTS_DEPTH) {
		return;
	}
	add_newlines(line, text, 2);
	++list_depth;
	list_levels[list_depth - 1].type = UNORDERED_LIST;
}

static inline void
ul_end_handler(struct line *line, struct wstring *text)
{
	add_newlines(line, text, 2);
	if (list_depth > 0) {
		--list_depth;
		line->indent = list_depth * SPACES_PER_INDENTATION_LEVEL;
	}
}

static inline void
ol_start_handler(struct line *line, struct wstring *text)
{
	if (list_depth == MAX_NESTED_LISTS_DEPTH) {
		return;
	}
	add_newlines(line, text, 2);
	++list_depth;
	list_levels[list_depth - 1].type = ORDERED_LIST;
	list_levels[list_depth - 1].length = 0;
}

static inline void
blockquote_start_handler(struct line *line, struct wstring *text)
{
	add_newlines(line, text, 2);
	line->indent += SPACES_PER_BLOCKQUOTE_LEVEL;
}

static inline void
blockquote_end_handler(struct line *line, struct wstring *text)
{
	add_newlines(line, text, 2);
	if (line->indent >= SPACES_PER_BLOCKQUOTE_LEVEL) {
		line->indent -= SPACES_PER_BLOCKQUOTE_LEVEL;
	}
}

static inline void
figure_start_handler(struct line *line, struct wstring *text)
{
	add_newlines(line, text, 2);
	line->indent += SPACES_PER_FIGURE_LEVEL;
}

static inline void
figure_end_handler(struct line *line, struct wstring *text)
{
	add_newlines(line, text, 2);
	if (line->indent >= SPACES_PER_FIGURE_LEVEL) {
		line->indent -= SPACES_PER_FIGURE_LEVEL;
	}
}

static inline void
pre_start_handler(struct line *line, struct wstring *text, enum html_position *pos)
{
	*pos |= HTML_PRE;
	add_newlines(line, text, 2);
}

static inline void
pre_end_handler(struct line *line, struct wstring *text, enum html_position *pos)
{
	*pos &= ~HTML_PRE;
	add_newlines(line, text, 2);
}

static inline bool
start_handler(wchar_t *t, struct line *l, struct wstring *w, enum html_position *p)
{
	     if (wcscmp(t, L"p")          == 0) { add_newlines(l, w, 2);          return true; }
	else if (wcscmp(t, L"details")    == 0) { add_newlines(l, w, 2);          return true; }
	else if (wcscmp(t, L"br")         == 0) { br_handler(l, w);               return true; }
	else if (wcscmp(t, L"li")         == 0) { li_start_handler(l, w);         return true; }
	else if (wcscmp(t, L"ul")         == 0) { ul_start_handler(l, w);         return true; }
	else if (wcscmp(t, L"ol")         == 0) { ol_start_handler(l, w);         return true; }
	else if (wcscmp(t, L"h1")         == 0) { add_newlines(l, w, 3);          return true; }
	else if (wcscmp(t, L"h2")         == 0) { add_newlines(l, w, 3);          return true; }
	else if (wcscmp(t, L"h3")         == 0) { add_newlines(l, w, 3);          return true; }
	else if (wcscmp(t, L"h4")         == 0) { add_newlines(l, w, 3);          return true; }
	else if (wcscmp(t, L"h5")         == 0) { add_newlines(l, w, 3);          return true; }
	else if (wcscmp(t, L"h6")         == 0) { add_newlines(l, w, 3);          return true; }
	else if (wcscmp(t, L"div")        == 0) { add_newlines(l, w, 1);          return true; }
	else if (wcscmp(t, L"section")    == 0) { add_newlines(l, w, 1);          return true; }
	else if (wcscmp(t, L"footer")     == 0) { add_newlines(l, w, 1);          return true; }
	else if (wcscmp(t, L"summary")    == 0) { add_newlines(l, w, 1);          return true; }
	else if (wcscmp(t, L"form")       == 0) { add_newlines(l, w, 1);          return true; }
	else if (wcscmp(t, L"hr")         == 0) { hr_handler(l, w);               return true; }
	else if (wcscmp(t, L"figcaption") == 0) { add_newlines(l, w, 1);          return true; }
	else if (wcscmp(t, L"figure")     == 0) { figure_start_handler(l, w);     return true; }
	else if (wcscmp(t, L"blockquote") == 0) { blockquote_start_handler(l, w); return true; }
	else if (wcscmp(t, L"pre")        == 0) { pre_start_handler(l, w, p);     return true; }

	return false;
}

static inline bool
end_handler(wchar_t *t, struct line *l, struct wstring *w, enum html_position *p)
{
	     if (wcscmp(t, L"p")          == 0) { add_newlines(l, w, 2);        return true; }
	else if (wcscmp(t, L"details")    == 0) { add_newlines(l, w, 2);        return true; }
	else if (wcscmp(t, L"li")         == 0) { add_newlines(l, w, 1);        return true; }
	else if (wcscmp(t, L"ul")         == 0) { ul_end_handler(l, w);         return true; }
	else if (wcscmp(t, L"ol")         == 0) { ul_end_handler(l, w);         return true; }
	else if (wcscmp(t, L"h1")         == 0) { add_newlines(l, w, 2);        return true; }
	else if (wcscmp(t, L"h2")         == 0) { add_newlines(l, w, 2);        return true; }
	else if (wcscmp(t, L"h3")         == 0) { add_newlines(l, w, 2);        return true; }
	else if (wcscmp(t, L"h4")         == 0) { add_newlines(l, w, 2);        return true; }
	else if (wcscmp(t, L"h5")         == 0) { add_newlines(l, w, 2);        return true; }
	else if (wcscmp(t, L"h6")         == 0) { add_newlines(l, w, 2);        return true; }
	else if (wcscmp(t, L"div")        == 0) { add_newlines(l, w, 1);        return true; }
	else if (wcscmp(t, L"section")    == 0) { add_newlines(l, w, 1);        return true; }
	else if (wcscmp(t, L"footer")     == 0) { add_newlines(l, w, 1);        return true; }
	else if (wcscmp(t, L"summary")    == 0) { add_newlines(l, w, 1);        return true; }
	else if (wcscmp(t, L"form")       == 0) { add_newlines(l, w, 1);        return true; }
	else if (wcscmp(t, L"figcaption") == 0) { add_newlines(l, w, 1);        return true; }
	else if (wcscmp(t, L"figure")     == 0) { figure_end_handler(l, w);     return true; }
	else if (wcscmp(t, L"blockquote") == 0) { blockquote_end_handler(l, w); return true; }
	else if (wcscmp(t, L"pre")        == 0) { pre_end_handler(l, w, p);     return true; }

	return false;
}

bool
render_text_html(const struct wstring *wstr, struct line *line, struct wstring *t, bool is_first_call)
{
	struct xml_tag *tag = create_tag();
	if (tag == NULL) {
		FAIL("Not enough memory for tag buffer to render HTML!");
		return false;
	}

	bool in_tag = false;
	bool in_entity = false;
	bool found_tag;

	wchar_t entity_name[MAX_ENTITY_NAME_LENGTH + 1];
	size_t entity_len;
	const wchar_t *entity_value;

	if (is_first_call == true) {
		list_depth = 0;
		html_pos = HTML_NONE;
	}

	const wchar_t *i = wstr->ptr;
	while (*i != L'\0') {
		if (in_entity == true) { // All entity characters go here.
			if (*i == L';') {
				in_entity = false;
				entity_name[entity_len] = L'\0';
				entity_value = translate_html_entity(entity_name);
				if (entity_value != NULL) {
					line_string(line, entity_value, t);
				} else {
					line_char(line, L'&', t);
					line_string(line, entity_name, t);
					line_char(line, L';', t);
				}
			} else {
				if (entity_len == MAX_ENTITY_NAME_LENGTH) {
					in_entity = false;
					entity_name[entity_len] = L'\0';
					line_char(line, L'&', t);
					line_string(line, entity_name, t);
				} else {
					entity_name[entity_len++] = *i;
				}
			}
		} else if (in_tag == true) { // All tag characters go here.
			if (append_wchar_to_tag(tag, *i) == XML_TAG_DONE) {
				found_tag = false;
				in_tag = false;
				if (tag->atts[0].name->ptr[0] == L'/') {
					found_tag = end_handler(tag->atts[0].name->ptr + 1, line, t, &html_pos);
				} else {
					found_tag = start_handler(tag->atts[0].name->ptr, line, t, &html_pos);
				}
				if (found_tag == false) {
					line_char(line, L'<', t);
					render_text_html(tag->buf, line, t, false);
					line_char(line, L'>', t);
				}
			}
		} else {
			if (*i == L'<') {
				in_tag = true;
				empty_tag(tag);
			} else if (*i == L'&') {
				in_entity = true;
				entity_len = 0;
			} else if (((html_pos & HTML_PRE) == 0) &&
			           ((*i == L' ') || (*i == L'\n') || (*i == L'\t')))
			{
				if ((line->len != 0) &&
					(line->ptr[line->len - 1] != L' ') &&
					(line->ptr[line->len - 1] != L'\n') &&
					(line->ptr[line->len - 1] != L'\t'))
				{
					if (line_char(line, L' ', t) == false) {
						goto error;
					}
				}
			} else {
				if (line_char(line, *i, t) == false) {
					goto error;
				}
			}
		}
		++i;
	}

	free_tag(tag);
	return true;

error:
	FAIL("Not enough memory for rendering HTML!");
	free_tag(tag);
	return false;
}
