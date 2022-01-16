#include <string.h>
#include "prepare_to_render_data/prepare_to_render_data.h"

struct data_handler {
	const char *const type;
	struct wstring *(*handle)(const struct wstring *, struct link_list *);
};

static const struct data_handler handlers[] = {
	// In many cases empty type means that text is HTML formatted.
	{"",              &prepare_to_render_text_html}, 
	{"text/html",     &prepare_to_render_text_html},
	{"html",          &prepare_to_render_text_html},
	//{"text/markdown"} TODO
	//{"text/x-rst"} TODO
};

bool
prepare_to_render_data(struct render_block *first_block, struct link_list *links)
{
	INFO("Preparing to render data.");
	struct string *new_content;
	struct wstring *converted_str;
	struct wstring *processed_str;
	struct render_block *block = first_block;
	while (block != NULL) {
		for (size_t i = 0; i < COUNTOF(handlers); ++i) {
			if (strcmp(block->content_type, handlers[i].type) == 0) {
				converted_str = convert_string_to_wstring(block->content);
				if (converted_str == NULL) {
					return false;
				}
				processed_str = handlers[i].handle(converted_str, links);
				if (processed_str == NULL) {
					free_wstring(converted_str);
					return false;
				}
				new_content = convert_wstring_to_string(processed_str);
				if (new_content == NULL) {
					free_wstring(processed_str);
					free_wstring(converted_str);
					return false;
				}
				free_string(block->content);
				block->content = new_content;
				free_wstring(processed_str);
				free_wstring(converted_str);
				break;
			}
		}
		block = block->next;
	}
	INFO("Finished preparing to render data.");
	return true;
}
