#include <stdlib.h>
#include <string.h>
#include <tidy.h>
#include <tidybuffio.h>
#include "render_data.h"

#define MAX_NESTED_LISTS_DEPTH 10
#define SPACES_PER_INDENTATION_LEVEL 4
#define SPACES_PER_BLOCKQUOTE_LEVEL 4
#define SPACES_PER_FIGURE_LEVEL 4

enum html_position {
	HTML_NONE = 0,
	HTML_TABLE = 1,
	HTML_TABLE_ROW = 2,
	HTML_TABLE_CELL = 4,
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
static struct html_table *table = NULL;

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
hr_handler(struct line *line, struct wstring *text)
{
	size_t temp_indent = line->indent;
	line->indent = 0;
	add_newlines(line, text, 1);
	for (size_t i = 1; i < line->lim; ++i) {
		line_char(line, L'─', text);
	}
	line_char(line, L'\n', text);
	line->indent = temp_indent;
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
cell_start_handler(enum html_position *pos)
{
	if ((*pos & HTML_TABLE_ROW) == 0) {
		return;
	}
	*pos |= HTML_TABLE_CELL;
	expand_last_row_in_html_table_by_one_cell(table);
}

static inline void
cell_end_handler(enum html_position *pos)
{
	*pos &= ~HTML_TABLE_CELL;
}

static inline void
row_start_handler(enum html_position *pos)
{
	if ((*pos & HTML_TABLE) == 0) {
		return;
	}
	*pos |= HTML_TABLE_ROW;
	expand_html_table_by_one_row(table);
}

static inline void
row_end_handler(enum html_position *pos)
{
	*pos &= ~HTML_TABLE_ROW;
}

static inline void
table_start_handler(enum html_position *pos)
{
	if (table != NULL) {
		// TODO NESTED TABLES
		return;
	}
	*pos |= HTML_TABLE;
	if ((table = create_html_table()) == NULL) {
		*pos &= ~HTML_TABLE;
	}
}

static inline void
table_end_handler(enum html_position *pos)
{
	*pos &= ~HTML_TABLE;
	/* print_html_table(table); */
	free_html_table(table);
	table = NULL;
}

static inline bool
start_handler(TidyTagId t, struct line *l, struct wstring *w, enum html_position *p)
{
	     if (t == TidyTag_P)          { add_newlines(l, w, 2);          return true; }
	else if (t == TidyTag_DETAILS)    { add_newlines(l, w, 2);          return true; }
	else if (t == TidyTag_BR)         { line_char(l, L'\n', w);         return true; }
	else if (t == TidyTag_LI)         { li_start_handler(l, w);         return true; }
	else if (t == TidyTag_UL)         { ul_start_handler(l, w);         return true; }
	else if (t == TidyTag_OL)         { ol_start_handler(l, w);         return true; }
	else if (t == TidyTag_H1)         { add_newlines(l, w, 3);          return true; }
	else if (t == TidyTag_H2)         { add_newlines(l, w, 3);          return true; }
	else if (t == TidyTag_H3)         { add_newlines(l, w, 3);          return true; }
	else if (t == TidyTag_H4)         { add_newlines(l, w, 3);          return true; }
	else if (t == TidyTag_H5)         { add_newlines(l, w, 3);          return true; }
	else if (t == TidyTag_H6)         { add_newlines(l, w, 3);          return true; }
	else if (t == TidyTag_DIV)        { add_newlines(l, w, 1);          return true; }
	else if (t == TidyTag_SECTION)    { add_newlines(l, w, 1);          return true; }
	else if (t == TidyTag_FOOTER)     { add_newlines(l, w, 1);          return true; }
	else if (t == TidyTag_SUMMARY)    { add_newlines(l, w, 1);          return true; }
	else if (t == TidyTag_FORM)       { add_newlines(l, w, 1);          return true; }
	else if (t == TidyTag_HR)         { hr_handler(l, w);               return true; }
	else if (t == TidyTag_FIGCAPTION) { add_newlines(l, w, 1);          return true; }
	else if (t == TidyTag_FIGURE)     { figure_start_handler(l, w);     return true; }
	else if (t == TidyTag_BLOCKQUOTE) { blockquote_start_handler(l, w); return true; }
	else if (t == TidyTag_TD)         { cell_start_handler(p);          return true; }
	else if (t == TidyTag_TH)         { cell_start_handler(p);          return true; }
	else if (t == TidyTag_TR)         { row_start_handler(p);           return true; }
	else if (t == TidyTag_TABLE)      { table_start_handler(p);         return true; }
	else if (t == TidyTag_PRE)        { add_newlines(l, w, 2);          return true; }
	else if (t == TidyTag_OPTION)     { add_newlines(l, w, 1);          return true; }

	return false;
}

static inline bool
end_handler(TidyTagId t, struct line *l, struct wstring *w, enum html_position *p)
{
	     if (t == TidyTag_P)          { add_newlines(l, w, 2);        return true; }
	else if (t == TidyTag_DETAILS)    { add_newlines(l, w, 2);        return true; }
	else if (t == TidyTag_LI)         { add_newlines(l, w, 1);        return true; }
	else if (t == TidyTag_UL)         { ul_end_handler(l, w);         return true; }
	else if (t == TidyTag_OL)         { ul_end_handler(l, w);         return true; }
	else if (t == TidyTag_H1)         { add_newlines(l, w, 2);        return true; }
	else if (t == TidyTag_H2)         { add_newlines(l, w, 2);        return true; }
	else if (t == TidyTag_H3)         { add_newlines(l, w, 2);        return true; }
	else if (t == TidyTag_H4)         { add_newlines(l, w, 2);        return true; }
	else if (t == TidyTag_H5)         { add_newlines(l, w, 2);        return true; }
	else if (t == TidyTag_H6)         { add_newlines(l, w, 2);        return true; }
	else if (t == TidyTag_DIV)        { add_newlines(l, w, 1);        return true; }
	else if (t == TidyTag_SECTION)    { add_newlines(l, w, 1);        return true; }
	else if (t == TidyTag_FOOTER)     { add_newlines(l, w, 1);        return true; }
	else if (t == TidyTag_SUMMARY)    { add_newlines(l, w, 1);        return true; }
	else if (t == TidyTag_FORM)       { add_newlines(l, w, 1);        return true; }
	else if (t == TidyTag_FIGCAPTION) { add_newlines(l, w, 1);        return true; }
	else if (t == TidyTag_FIGURE)     { figure_end_handler(l, w);     return true; }
	else if (t == TidyTag_BLOCKQUOTE) { blockquote_end_handler(l, w); return true; }
	else if (t == TidyTag_TD)         { cell_end_handler(p);          return true; }
	else if (t == TidyTag_TH)         { cell_end_handler(p);          return true; }
	else if (t == TidyTag_TR)         { row_end_handler(p);           return true; }
	else if (t == TidyTag_TABLE)      { table_end_handler(p);         return true; }
	else if (t == TidyTag_PRE)        { add_newlines(l, w, 2);        return true; }
	else if (t == TidyTag_OPTION)     { add_newlines(l, w, 1);        return true; }

	return false;
}

static void
cat_tag_to_line(struct line *line, const char *tag_name, TidyAttr *atts, bool is_start)
{
	(void)line;
	(void)tag_name;
	(void)atts;
	(void)is_start;
	return;
}

static void
dumpNode(TidyDoc *tdoc, TidyNode tnod, TidyBuffer *buf, struct line *line, struct wstring *text, enum html_position *pos)
{
	const char *child_name;
	TidyTagId child_id;
	TidyAttr child_atts;
	for (TidyNode child = tidyGetChild(tnod); child; child = tidyGetNext(child)) {
		child_name = tidyNodeGetName(child);
		child_id = tidyNodeGetId(child);
		child_atts = tidyAttrFirst(child);
		if (start_handler(child_id, line, text, pos) == false) {
			cat_tag_to_line(line, child_name, &child_atts, true);
		}
		if (tidyNodeGetType(child) == TidyNode_Text) {
			if (tidyNodeHasText(*tdoc, child) == true) {
				tidyBufClear(buf);
				tidyNodeGetValue(*tdoc, child, buf);
				if (buf->bp != NULL) {
					struct string *str = crtas((char *)buf->bp, strlen((char*)buf->bp));
					if (str != NULL) {
						struct wstring *wstr = convert_string_to_wstring(str);
						free_string(str);
						if (wstr != NULL) {
							line_string(line, wstr->ptr, text);
							free_wstring(wstr);
						}
					}
				}
			}
		}
		dumpNode(tdoc, child, buf, line, text, pos);
		if (end_handler(child_id, line, text, pos) == false) {
			cat_tag_to_line(line, child_name, &child_atts, false);
		}
	}
}

bool
render_text_html(const struct wstring *wstr, struct line *text_line, struct wstring *text, bool is_first_call)
{
	enum html_position html_pos = HTML_NONE;
	if (is_first_call == true) {
		list_depth = 0;
	}

	TidyDoc tdoc = tidyCreate();
	TidyBuffer tempbuf = {0};
	TidyBuffer tidy_errbuf = {0};

	tidyBufInit(&tempbuf);

	tidySetErrorBuffer(tdoc, &tidy_errbuf);
	tidyOptSetInt(tdoc, TidyWrapLen, 0); // disable wrapping
	tidyOptSetBool(tdoc, TidyMakeBare, true); // use plain quotes instead fancy ones
	tidyOptSetBool(tdoc, TidyOmitOptionalTags, false);
	tidyOptSetBool(tdoc, TidyCoerceEndTags, true);

	// Convert entities to characters.
	tidyOptSetBool(tdoc, TidyAsciiChars, true);
	tidyOptSetBool(tdoc, TidyQuoteNbsp, false);
	tidyOptSetBool(tdoc, TidyQuoteMarks, false);
	tidyOptSetBool(tdoc, TidyQuoteAmpersand, false);
	tidyOptSetBool(tdoc, TidyPreserveEntities, false);

	struct string *str = convert_wstring_to_string(wstr);
	tidyParseString(tdoc, str->ptr);
	tidyCleanAndRepair(tdoc);
	tidyRunDiagnostics(tdoc);

	dumpNode(&tdoc, tidyGetBody(tdoc), &tempbuf, text_line, text, &html_pos);

	if (tidy_errbuf.bp != NULL) {
		INFO("Tidy report:\n%s", tidy_errbuf.bp);
	} else {
		INFO("Tidy run silently.");
	}

	tidyBufFree(&tempbuf);
	tidyBufFree(&tidy_errbuf);
	tidyRelease(tdoc);
	free_string(str);
	return true;
}
