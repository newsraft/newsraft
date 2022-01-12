#include "extract_links.h"

struct data_handler {
	const char *const type;
	bool (*handle)(const struct wstring *, struct link_list *);
};

static const struct data_handler handlers[] = {
	{"", &extract_from_html}, // In many cases empty type means that
	                          // text is HTML formatted.
	{"text/html", &extract_from_html},
	{"html", &extract_from_html},
	//{"text/markdown"} TODO
	//{"text/x-rst"} TODO
};

bool
extract_links(const struct render_block *first_block, struct link_list *target)
{
	INFO("Extracting links from content list.");
	const struct render_block *block = first_block;
	struct wstring *wstr;
	while (block != NULL) {
		for (size_t i = 0; i < COUNTOF(handlers); ++i) {
			if (strcmp(block->content_type, handlers[i].type) == 0) {
				wstr = convert_string_to_wstring(block->content);
				if (wstr == NULL) {
					return false;
				}
				if (handlers[i].handle(wstr, target) == false) {
					free_wstring(wstr);
					return false;
				}
				free_wstring(wstr);
				break;
			}
		}
		block = block->next;
	}
	INFO("Links extraction finished.");
	return true;
}
