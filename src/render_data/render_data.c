#include <string.h>
#include "render_data.h"

struct data_handler {
	const char *const type;
	struct wstring *(*handle)(const struct wstring *);
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
	//{"text/markdown"} TODO
	//{"text/x-rst"} TODO
};

struct wstring *
render_data(const struct content_list *data_list)
{
	struct wstring *text = create_empty_wstring();
	if (text == NULL) {
		FAIL("Not enough memory to render data!");
		return NULL;
	}
	struct wstring *converted_str;
	struct wstring *temp_str;
	const struct content_list *temp_list = data_list;
	bool found_handler;
	while (temp_list != NULL) {
		found_handler = false;
		converted_str = convert_string_to_wstring(temp_list->content);
		if (converted_str == NULL) {
			free_wstring(text);
			return NULL;
		}
		for (size_t i = 0; i < LENGTH(handlers); ++i) {
			if (strcmp(temp_list->content_type, handlers[i].type) == 0) {
				found_handler = true;
				temp_str = handlers[i].handle(converted_str);
				break;
			}
		}
		if (found_handler == false) {
			temp_str = render_text_plain(converted_str);
		}
		if (temp_str != NULL) {
			wcatss(text, temp_str);
			free_wstring(temp_str);
		}
		free_wstring(converted_str);
		temp_list = temp_list->next;
	}
	return text;
}
