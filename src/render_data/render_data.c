#include <string.h>
#include "render_data.h"

struct data_handler {
	const char *const type;
	struct wstring *(*handle)(const struct wstring *, struct line *);
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
	{"text/html", &render_text_html},
	{"html", &render_text_html},
	//{"text/markdown"} TODO
	//{"text/x-rst"} TODO
};

struct wstring *
render_data(const struct content_list *data_list)
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
	struct wstring *temp_str;
	const struct content_list *temp_list = data_list;
	bool found_handler;
	while (temp_list != NULL) {
		converted_str = convert_string_to_wstring(temp_list->content);
		if (converted_str == NULL) {
			free(line.ptr);
			free_wstring(text);
			return NULL;
		}
		found_handler = false;
		for (size_t i = 0; i < COUNTOF(handlers); ++i) {
			if (strcmp(temp_list->content_type, handlers[i].type) == 0) {
				found_handler = true;
				temp_str = handlers[i].handle(converted_str, &line);
				break;
			}
		}
		if (found_handler == false) {
			temp_str = render_text_plain(converted_str, &line);
		}
		if (temp_str != NULL) {
			if (temp_list->trim_whitespace == true) {
				trim_whitespace_from_wstring(temp_str);
			}
			wcatss(text, temp_str);
			free_wstring(temp_str);
		}
		free_wstring(converted_str);
		temp_list = temp_list->next;
	}
	if (line.len != 0) {
		// Squeeze out line remainings to text by sending newline character.
		line_char(&line, L'\n', text);
	}
	free(line.ptr);
	trim_whitespace_from_wstring(text);
	// We still need last newline to count in last line of text.
	wcatcs(text, L'\n');
	return text;
}
