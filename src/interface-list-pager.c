#include <stdlib.h>
#include "newsraft.h"

struct wstring *content = NULL;
size_t content_height;
static struct render_blocks_list *blocks;

bool
pager_menu_moderator(size_t index)
{
	return index < content_height;
}

void
pager_list_menu_writer(size_t index, WINDOW *w)
{
	if (index < content_height) {
		size_t line_index = 0;
		const wchar_t *first_char = content->ptr;
		const wchar_t *last_char = wcschr(content->ptr, L'\n');
		while (line_index < index) {
			first_char = last_char + 1;
			last_char = wcschr(first_char, L'\n');
			if (last_char == NULL) {
				return;
			}
			line_index += 1;
		}
		const size_t first_index = first_char - content->ptr;
		const size_t last_index = last_char - content->ptr;
		for (size_t i = 0, hint_index = 0; i < last_index; ++i) {
			if (hint_index < blocks->hints_len && i == blocks->hints[hint_index].pos) {
				if (blocks->hints[hint_index].value & FORMAT_BOLD_END) {
					wattroff(w, A_BOLD);
				} else if (blocks->hints[hint_index].value & FORMAT_BOLD_BEGIN) {
					wattron(w, A_BOLD);
				}
				if (blocks->hints[hint_index].value & FORMAT_UNDERLINED_END) {
					wattroff(w, A_UNDERLINE);
				} else if (blocks->hints[hint_index].value & FORMAT_UNDERLINED_BEGIN) {
					wattron(w, A_UNDERLINE);
				}
#ifdef A_ITALIC // Since A_ITALIC is an ncurses extension, some systems may lack it.
				if (blocks->hints[hint_index].value & FORMAT_ITALIC_END) {
					wattroff(w, A_ITALIC);
				} else if (blocks->hints[hint_index].value & FORMAT_ITALIC_BEGIN) {
					wattron(w, A_ITALIC);
				}
#endif
				hint_index += 1;
			}
			if (i >= first_index) {
				waddnwstr(w, content->ptr + i, 1);
			}
		}
	}
}

bool
start_pager_menu(struct render_blocks_list *new_blocks)
{
	blocks = new_blocks;
	return reset_pager_menu();
}

bool
reset_pager_menu(void)
{
	free_wstring(content);
	free(blocks->hints);
	blocks->hints = NULL;
	blocks->hints_len = 0;
	content = render_data(blocks);
	if (content == NULL || content->len == 0) {
		fail_status("Can't render content!");
		free_wstring(content);
		return false;
	}
	// As long as rendered data was generated according to size of the terminal and
	// every line in it is terminated with newline, we can get required height for
	// pad window by counting newline characters in the rendered text wstring.
	content_height = 0;
	for (size_t i = 0; i < content->len; ++i) {
		if (content->ptr[i] == L'\n') {
			content_height += 1;
		}
	}
	return true;
}
