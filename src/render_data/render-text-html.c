#include <stdlib.h>
#include <string.h>
#include "render_data.h"

#define MAX_NESTED_LISTS_DEPTH 10
#define SPACES_PER_INDENTATION_LEVEL 4

enum html_position {
	HTML_NONE = 0,
	HTML_PRE = 1,
	HTML_STYLE = 2,
	HTML_SCRIPT = 4,
};

enum list_type {
	UNORDERED_LIST,
	ORDERED_LIST,
};

struct list_level {
	enum list_type type;
	uint16_t length;
};

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
		line_string(line, text, L"*  ");;
		line->indent += 3;
	} else {
		++(list_levels[list_depth - 1].length);
		// 9 = 5 (for longest uint16_t) + 2 (for dot and space) + 1 (for terminator) + 1 (for luck lol)
		wchar_t number_str[9];
		size_t number_str_len = swprintf(number_str, 9, L"%d. ", list_levels[list_depth - 1].length);
		line_string(line, text, number_str);
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
sup_start_handler(struct line *line, struct wstring *text)
{
	line_char(line, L'^', text);
}

static inline void
q_start_handler(struct line *line, struct wstring *text)
{
	line_char(line, L'"', text);
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

static inline void
script_start_handler(enum html_position *pos)
{
	*pos |= HTML_SCRIPT;
}

static inline void
script_end_handler(enum html_position *pos)
{
	*pos &= ~HTML_SCRIPT;
}

static inline void
style_start_handler(enum html_position *pos)
{
	*pos |= HTML_STYLE;
}

static inline void
style_end_handler(enum html_position *pos)
{
	*pos &= ~HTML_STYLE;
}

static inline bool
start_handler(wchar_t *t, struct line *l, struct wstring *w, enum html_position *p)
{
	     if (wcscmp(t, L"span")       == 0) { /* just nothing */          return true; }
	else if (wcscmp(t, L"p")          == 0) { add_newlines(l, w, 2);      return true; }
	else if (wcscmp(t, L"details")    == 0) { add_newlines(l, w, 2);      return true; }
	else if (wcscmp(t, L"br")         == 0) { br_handler(l, w);           return true; }
	else if (wcscmp(t, L"li")         == 0) { li_start_handler(l, w);     return true; }
	else if (wcscmp(t, L"ul")         == 0) { ul_start_handler(l, w);     return true; }
	else if (wcscmp(t, L"ol")         == 0) { ol_start_handler(l, w);     return true; }
	else if (wcscmp(t, L"h1")         == 0) { add_newlines(l, w, 3);      return true; }
	else if (wcscmp(t, L"h2")         == 0) { add_newlines(l, w, 3);      return true; }
	else if (wcscmp(t, L"h3")         == 0) { add_newlines(l, w, 3);      return true; }
	else if (wcscmp(t, L"h4")         == 0) { add_newlines(l, w, 3);      return true; }
	else if (wcscmp(t, L"h5")         == 0) { add_newlines(l, w, 3);      return true; }
	else if (wcscmp(t, L"h6")         == 0) { add_newlines(l, w, 3);      return true; }
	else if (wcscmp(t, L"div")        == 0) { add_newlines(l, w, 1);      return true; }
	else if (wcscmp(t, L"section")    == 0) { add_newlines(l, w, 1);      return true; }
	else if (wcscmp(t, L"footer")     == 0) { add_newlines(l, w, 1);      return true; }
	else if (wcscmp(t, L"summary")    == 0) { add_newlines(l, w, 1);      return true; }
	else if (wcscmp(t, L"sup")        == 0) { sup_start_handler(l, w);    return true; }
	else if (wcscmp(t, L"hr")         == 0) { hr_handler(l, w);           return true; }
	else if (wcscmp(t, L"figcaption") == 0) { add_newlines(l, w, 1);      return true; }
	else if (wcscmp(t, L"figure")     == 0) { add_newlines(l, w, 2);      return true; }
	else if (wcscmp(t, L"blockquote") == 0) { add_newlines(l, w, 2);      return true; }
	else if (wcscmp(t, L"q")          == 0) { q_start_handler(l, w);      return true; }
	else if (wcscmp(t, L"code")       == 0) { /* TODO */                  return true; }
	else if (wcscmp(t, L"a")          == 0) { /* TODO */                  return true; }
	else if (wcscmp(t, L"b")          == 0) { /* TODO */                  return true; }
	else if (wcscmp(t, L"i")          == 0) { /* TODO */                  return true; }
	else if (wcscmp(t, L"em")         == 0) { /* TODO */                  return true; }
	else if (wcscmp(t, L"mark")       == 0) { /* TODO */                  return true; }
	else if (wcscmp(t, L"small")      == 0) { /* TODO */                  return true; }
	else if (wcscmp(t, L"time")       == 0) { /* TODO */                  return true; }
	else if (wcscmp(t, L"strong")     == 0) { /* TODO */                  return true; }
	else if (wcscmp(t, L"pre")        == 0) { pre_start_handler(l, w, p); return true; }
	else if (wcscmp(t, L"script")     == 0) { script_start_handler(p);    return true; }
	else if (wcscmp(t, L"style")      == 0) { style_start_handler(p);     return true; }

	return false;
}

static inline bool
end_handler(wchar_t *t, struct line *l, struct wstring *w, enum html_position *p)
{
	     if (wcscmp(t, L"span")       == 0) { /* just nothing */        return true; }
	else if (wcscmp(t, L"p")          == 0) { add_newlines(l, w, 2);    return true; }
	else if (wcscmp(t, L"details")    == 0) { add_newlines(l, w, 2);    return true; }
	else if (wcscmp(t, L"li")         == 0) { add_newlines(l, w, 1);    return true; }
	else if (wcscmp(t, L"ul")         == 0) { ul_end_handler(l, w);     return true; }
	else if (wcscmp(t, L"ol")         == 0) { ul_end_handler(l, w);     return true; }
	else if (wcscmp(t, L"h1")         == 0) { add_newlines(l, w, 2);    return true; }
	else if (wcscmp(t, L"h2")         == 0) { add_newlines(l, w, 2);    return true; }
	else if (wcscmp(t, L"h3")         == 0) { add_newlines(l, w, 2);    return true; }
	else if (wcscmp(t, L"h4")         == 0) { add_newlines(l, w, 2);    return true; }
	else if (wcscmp(t, L"h5")         == 0) { add_newlines(l, w, 2);    return true; }
	else if (wcscmp(t, L"h6")         == 0) { add_newlines(l, w, 2);    return true; }
	else if (wcscmp(t, L"div")        == 0) { add_newlines(l, w, 1);    return true; }
	else if (wcscmp(t, L"section")    == 0) { add_newlines(l, w, 1);    return true; }
	else if (wcscmp(t, L"footer")     == 0) { add_newlines(l, w, 1);    return true; }
	else if (wcscmp(t, L"summary")    == 0) { add_newlines(l, w, 1);    return true; }
	else if (wcscmp(t, L"sup")        == 0) { /* just nothing */        return true; }
	else if (wcscmp(t, L"figcaption") == 0) { add_newlines(l, w, 1);    return true; }
	else if (wcscmp(t, L"figure")     == 0) { add_newlines(l, w, 2);    return true; }
	else if (wcscmp(t, L"blockquote") == 0) { add_newlines(l, w, 2);    return true; }
	else if (wcscmp(t, L"q")          == 0) { q_start_handler(l, w);    return true; }
	else if (wcscmp(t, L"code")       == 0) { /* TODO */                return true; }
	else if (wcscmp(t, L"a")          == 0) { /* TODO */                return true; }
	else if (wcscmp(t, L"b")          == 0) { /* TODO */                return true; }
	else if (wcscmp(t, L"i")          == 0) { /* TODO */                return true; }
	else if (wcscmp(t, L"em")         == 0) { /* TODO */                return true; }
	else if (wcscmp(t, L"mark")       == 0) { /* TODO */                return true; }
	else if (wcscmp(t, L"small")      == 0) { /* TODO */                return true; }
	else if (wcscmp(t, L"time")       == 0) { /* TODO */                return true; }
	else if (wcscmp(t, L"strong")     == 0) { /* TODO */                return true; }
	else if (wcscmp(t, L"pre")        == 0) { pre_end_handler(l, w, p); return true; }
	else if (wcscmp(t, L"script")     == 0) { script_end_handler(p);    return true; }
	else if (wcscmp(t, L"style")      == 0) { style_end_handler(p);     return true; }

	return false;
}

// Append some characters according to the current position in the HTML document.
static inline int
format_text(struct wstring *tag, struct line *line, struct wstring *text, enum html_position *pos)
{
	wchar_t *tag_name;
	bool is_end_tag;

	if (tag->ptr[0] == L'/') {
		tag_name = tag->ptr + 1;
		is_end_tag = true;
	} else {
		tag_name = tag->ptr;
		is_end_tag = false;
	}

	size_t tag_name_len = 0;
	for (size_t i = 0; i < tag->len; ++i) {
		if ((tag->ptr[i] == L' ') || (tag->ptr[i] == L'\n') || (tag->ptr[i] == L'\t')) {
			break;
		}
		++tag_name_len;
	}

	wchar_t temp = tag_name[tag_name_len];
	tag_name[tag_name_len] = L'\0';

	if (is_end_tag == true) {
		if (end_handler(tag_name, line, text, pos) == true) {
			return 0;
		}
	} else {
		if (start_handler(tag_name, line, text, pos) == true) {
			return 0;
		}
	}

	tag_name[tag_name_len] = temp;

	line_char(line, L'<', text);
	line_string(line, text, tag->ptr);
	line_char(line, L'>', text);

	return 0;
}

struct wstring *
render_text_html(const struct wstring *wstr, struct line *line)
{
	struct wstring *t = create_wstring(NULL, wstr->len);
	if (t == NULL) {
		FAIL("Not enough memory for text buffer to render HTML!");
		return NULL;
	}

	struct wstring *tag = create_wstring(NULL, 100);
	if (tag == NULL) {
		FAIL("Not enough memory for tag buffer to render HTML!");
		free_wstring(t);
		return NULL;
	}

	bool in_tag = false;
	bool in_entity = false;
	bool error = false;

	wchar_t entity_name[MAX_ENTITY_NAME_LENGTH + 1];
	size_t entity_len;
	const wchar_t *entity_value;

	list_depth = 0;
	enum html_position position = HTML_NONE;

	const wchar_t *i = wstr->ptr;
	while (*i != L'\0') {
		if (in_tag == false) {
			if (in_entity == false) {
				if (*i == L'<') {
					in_tag = true;
					empty_wstring(tag);
				} else if (*i == L'&') {
					in_entity = true;
					entity_len = 0;
				} else if ((position & (HTML_STYLE|HTML_SCRIPT)) != 0) {
					// Ignore contents of style and script elements.
					++i;
					continue;
				} else if (((position & HTML_PRE) == 0) &&
						((*i == L' ') || (*i == L'\n') || (*i == L'\t')))
				{
					if ((line->len != 0) &&
						(line->ptr[line->len - 1] != L' ') &&
						(line->ptr[line->len - 1] != L'\n') &&
						(line->ptr[line->len - 1] != L'\t'))
					{
						if (line_char(line, L' ', t) != 0) {
							error = true;
							break;
						}
					}
				} else {
					if (line_char(line, *i, t) != 0) {
						error = true;
						break;
					}
				}
			} else { // All entity characters go here.
				if (*i == L';') {
					in_entity = false;
					entity_name[entity_len] = L'\0';
					entity_value = translate_html_entity(entity_name);
					if (entity_value != NULL) {
						line_string(line, t, entity_value);
					} else {
						line_char(line, L'&', t);
						line_string(line, t, entity_name);
						line_char(line, L';', t);
					}
				} else {
					if (entity_len == MAX_ENTITY_NAME_LENGTH) {
						in_entity = false;
						entity_name[entity_len] = L'\0';
						line_char(line, L'&', t);
						line_string(line, t, entity_name);
					} else {
						entity_name[entity_len++] = *i;
					}
				}
			}
		} else { // All tag characters go here.
			if (*i == L'>') {
				in_tag = false;
				format_text(tag, line, t, &position);
			} else {
				if (wcatcs(tag, *i) == false) {
					error = true;
					break;
				}
			}
		}
		++i;
	}

	free_wstring(tag);

	if (error == true) {
		FAIL("Not enough memory for rendering HTML!");
		free_wstring(t);
		return NULL;
	}

	return t;
}
