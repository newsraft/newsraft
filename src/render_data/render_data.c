#include <string.h>
#include "render_data.h"

struct data_handler {
	const char *const type;
	bool (*handle)(const struct wstring *, struct line *, struct wstring *, bool);
};

// Besides formatting according to the content type, these handlers must
// split whole character data into pieces N characters long (at maximum)
// divided by newlines, where N is the current width of the terminal.
// Last character of the resulting wstring must be a newline too.
// It is made that way so that we can quickly count number of lines needed
// for pager pad window to store content.
//
// You can't find "text/plain" type here because plain text handler is the
// default one and will be used only if none of these triggered.
static const struct data_handler handlers[] = {
	{"", &render_text_html}, // In many cases empty type means that
	                         // text is HTML formatted.
	{"text/plain", &render_text_plain},
	{"text/html", &render_text_html},
	{"html", &render_text_html},
	//{"text/markdown"} TODO
	//{"text/x-rst"} TODO
};

struct wstring *
render_data(const struct render_block *first_block)
{
	struct wstring *text = wcrtes();
	if (text == NULL) {
		FAIL("Not enough memory to render data!");
		return NULL;
	}
	struct line line;
	line.ptr = malloc(sizeof(wchar_t) * list_menu_width);
	if (line.ptr == NULL) {
		FAIL("Not enough memory for line buffer to render data!");
		free_wstring(text);
		return NULL;
	}
	line.len = 0;
	line.lim = list_menu_width;
	line.pin = SIZE_MAX;
	line.indent = 0;
	struct wstring *converted_str;
	const struct render_block *block = first_block;
	bool found_handler;
	while (block != NULL) {
		converted_str = convert_string_to_wstring(block->content);
		if (converted_str == NULL) {
			free(line.ptr);
			free_wstring(text);
			return NULL;
		}
		found_handler = false;
		for (size_t i = 0; i < COUNTOF(handlers); ++i) {
			if (strcmp(block->content_type, handlers[i].type) == 0) {
				found_handler = true;
				handlers[i].handle(converted_str, &line, text, true);
				if (line.len == 0) {
					trim_whitespace_from_wstring(text);
				}
				break;
			}
		}
		if (found_handler == false) {
			// This is prolly a separator, so don't trim whitespace!
			line_string(&line, converted_str->ptr, text);
		}
		free_wstring(converted_str);
		block = block->next;
	}
	for (size_t i = 0; i < line.len; ++i) {
		wcatcs(text, line.ptr[i]);
	}
	free(line.ptr);
	trim_whitespace_from_wstring(text);
	// We still need last newline to count in last line of text.
	wcatcs(text, L'\n');
	return text;
}
