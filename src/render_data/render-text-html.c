#include <stdlib.h>
#include <string.h>
#include <tidy.h>
#include <tidybuffio.h>
#include "render_data.h"

struct html_element_handler {
	const TidyTagId tag_id;
	void (*start_handler)(struct wstring *, struct line *);
	void (*end_handler)(struct wstring *, struct line *);
};

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

static inline void
provide_newlines(struct wstring *text, struct line *line, int8_t count)
{
	int8_t how_many_newlines_already_present = 0;
	if (line->len == 0) {
		for (int8_t i = 1; i <= count; ++i) {
			if ((text->len >= (size_t)i) && (text->ptr[text->len - i] == L'\n')) {
				++how_many_newlines_already_present;
			} else {
				break;
			}
		}
	}
	for (int8_t i = 0; i < count - how_many_newlines_already_present; ++i) {
		line_char(line, L'\n', text);
	}
}

static void
provide_one_newline(struct wstring *text, struct line *line)
{
	provide_newlines(text, line, 1);
}

static void
provide_two_newlines(struct wstring *text, struct line *line)
{
	provide_newlines(text, line, 2);
}

static void
provide_three_newlines(struct wstring *text, struct line *line)
{
	provide_newlines(text, line, 3);
}

static void
br_handler(struct wstring *text, struct line *line)
{
	line_char(line, L'\n', text);
}

static void
hr_handler(struct wstring *text, struct line *line)
{
	size_t temp_indent = line->indent;
	line->indent = 0;
	provide_one_newline(text, line);
	for (size_t i = 1; i < line->lim; ++i) {
		line_char(line, L'─', text);
	}
	line_char(line, L'\n', text);
	line->indent = temp_indent;
}

static void
li_handler(struct wstring *text, struct line *line)
{
	provide_one_newline(text, line);
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

static void
ul_start_handler(struct wstring *text, struct line *line)
{
	if (list_depth == MAX_NESTED_LISTS_DEPTH) {
		return;
	}
	provide_two_newlines(text, line);
	++list_depth;
	list_levels[list_depth - 1].type = UNORDERED_LIST;
}

static void
ul_end_handler(struct wstring *text, struct line *line)
{
	provide_two_newlines(text, line);
	if (list_depth > 0) {
		--list_depth;
		line->indent = list_depth * SPACES_PER_INDENTATION_LEVEL;
	}
}

static void
ol_start_handler(struct wstring *text, struct line *line)
{
	if (list_depth == MAX_NESTED_LISTS_DEPTH) {
		return;
	}
	provide_two_newlines(text, line);
	++list_depth;
	list_levels[list_depth - 1].type = ORDERED_LIST;
	list_levels[list_depth - 1].length = 0;
}

static void
blockquote_start_handler(struct wstring *text, struct line *line)
{
	provide_two_newlines(text, line);
	line->indent += SPACES_PER_BLOCKQUOTE_LEVEL;
}

static void
blockquote_end_handler(struct wstring *text, struct line *line)
{
	provide_two_newlines(text, line);
	if (line->indent >= SPACES_PER_BLOCKQUOTE_LEVEL) {
		line->indent -= SPACES_PER_BLOCKQUOTE_LEVEL;
	}
}

static void
figure_start_handler(struct wstring *text, struct line *line)
{
	provide_two_newlines(text, line);
	line->indent += SPACES_PER_FIGURE_LEVEL;
}

static void
figure_end_handler(struct wstring *text, struct line *line)
{
	provide_two_newlines(text, line);
	if (line->indent >= SPACES_PER_FIGURE_LEVEL) {
		line->indent -= SPACES_PER_FIGURE_LEVEL;
	}
}

static const struct html_element_handler handlers[] = {
	{TidyTag_P,          &provide_two_newlines,     &provide_two_newlines},
	{TidyTag_BR,         &br_handler,               NULL},
	{TidyTag_LI,         &li_handler,               &provide_one_newline},
	{TidyTag_UL,         &ul_start_handler,         &ul_end_handler},
	{TidyTag_OL,         &ol_start_handler,         &ul_end_handler},
	{TidyTag_PRE,        &provide_two_newlines,     &provide_two_newlines},
	{TidyTag_H1,         &provide_three_newlines,   &provide_two_newlines},
	{TidyTag_H2,         &provide_three_newlines,   &provide_two_newlines},
	{TidyTag_H3,         &provide_three_newlines,   &provide_two_newlines},
	{TidyTag_H4,         &provide_three_newlines,   &provide_two_newlines},
	{TidyTag_H5,         &provide_three_newlines,   &provide_two_newlines},
	{TidyTag_H6,         &provide_three_newlines,   &provide_two_newlines},
	{TidyTag_HR,         &hr_handler,               NULL},
	{TidyTag_FIGURE,     &figure_start_handler,     &figure_end_handler},
	{TidyTag_BLOCKQUOTE, &blockquote_start_handler, &blockquote_end_handler},
	{TidyTag_DIV,        &provide_one_newline,      &provide_one_newline},
	{TidyTag_SUMMARY,    &provide_one_newline,      &provide_one_newline},
	{TidyTag_DETAILS,    &provide_two_newlines,     &provide_two_newlines},
	{TidyTag_FIGCAPTION, &provide_one_newline,      &provide_one_newline},
	{TidyTag_SECTION,    &provide_one_newline,      &provide_one_newline},
	{TidyTag_FOOTER,     &provide_one_newline,      &provide_one_newline},
	{TidyTag_OPTION,     &provide_one_newline,      &provide_one_newline},
	{TidyTag_FORM,       &provide_one_newline,      &provide_one_newline},
	{TidyTag_UNKNOWN,    NULL,                      NULL},
};

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
	TidyNodeType child_type;
	TidyAttr child_atts;
	size_t i;
	for (TidyNode child = tidyGetChild(tnod); child; child = tidyGetNext(child)) {
		child_type = tidyNodeGetType(child);
		if ((child_type == TidyNode_Text) || (child_type == TidyNode_CDATA)) {
			tidyBufClear(buf);
			tidyNodeGetValue(*tdoc, child, buf);
			if (buf->bp != NULL) {
				struct string *str = crtas((char *)buf->bp, buf->size);
				if (str != NULL) {
					struct wstring *wstr = convert_string_to_wstring(str);
					free_string(str);
					if (wstr != NULL) {
						line_string(line, wstr->ptr, text);
						free_wstring(wstr);
					}
				}
			}
		} else if ((child_type == TidyNode_Start) || (child_type == TidyNode_StartEnd)) {
			child_id = tidyNodeGetId(child);
			for (i = 0; handlers[i].tag_id != TidyTag_UNKNOWN; ++i) {
				if (child_id == handlers[i].tag_id) {
					break;
				}
			}
			if (handlers[i].tag_id == TidyTag_UNKNOWN) {
				child_name = tidyNodeGetName(child);
				child_atts = tidyAttrFirst(child);
				cat_tag_to_line(line, child_name, &child_atts, true);
				dumpNode(tdoc, child, buf, line, text, pos);
				cat_tag_to_line(line, child_name, &child_atts, false);
			} else {
				if (handlers[i].start_handler != NULL) {
					handlers[i].start_handler(text, line);
				}
				dumpNode(tdoc, child, buf, line, text, pos);
				if (handlers[i].end_handler != NULL) {
					handlers[i].end_handler(text, line);
				}
			}
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
