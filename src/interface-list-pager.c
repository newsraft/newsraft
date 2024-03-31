#include <stdlib.h>
#include "newsraft.h"

static struct config_context **pager_context = NULL;
static struct render_result content = {0};
static struct render_blocks_list *pager_blocks = NULL;

bool
is_pager_pos_valid(struct menu_state *ctx, size_t index)
{
	(void)ctx;
	return index < content.lines_len;
}

void
pager_menu_writer(size_t index, WINDOW *w)
{
	if (index < content.lines_len) {
		for (size_t i = 0; i < content.lines[index].indent; ++i) {
			waddnwstr(w, L" ", 1);
		}
		for (size_t i = 0, hint_index = 0; i < content.lines[index].ws->len; ++i) {
			if (hint_index < content.lines[index].hints_len && i == content.lines[index].hints[hint_index].pos) {
				if (content.lines[index].hints[hint_index].mask & FORMAT_BOLD_END) {
					wattroff(w, A_BOLD);
				} else if (content.lines[index].hints[hint_index].mask & FORMAT_BOLD_BEGIN) {
					wattron(w, A_BOLD);
				}
				if (content.lines[index].hints[hint_index].mask & FORMAT_UNDERLINED_END) {
					wattroff(w, A_UNDERLINE);
				} else if (content.lines[index].hints[hint_index].mask & FORMAT_UNDERLINED_BEGIN) {
					wattron(w, A_UNDERLINE);
				}
#ifdef A_ITALIC // Since A_ITALIC is an ncurses extension, some systems may lack it.
				if (content.lines[index].hints[hint_index].mask & FORMAT_ITALIC_END) {
					wattroff(w, A_ITALIC);
				} else if (content.lines[index].hints[hint_index].mask & FORMAT_ITALIC_BEGIN) {
					wattron(w, A_ITALIC);
				}
#endif
				hint_index += 1;
			}
			waddnwstr(w, content.lines[index].ws->ptr + i, 1);
		}
	}
}

bool
start_pager_menu(struct config_context **new_ctx, struct render_blocks_list *new_blocks)
{
	pager_context = new_ctx;
	pager_blocks = new_blocks;
	return refresh_pager_menu();
}

bool
refresh_pager_menu(void)
{
	for (size_t i = 0; i < content.lines_len; ++i) {
		free_wstring(content.lines[i].ws);
		free(content.lines[i].hints);
	}
	free(content.lines);
	content.lines = NULL;
	content.lines_len = 0;
	return render_data(pager_context, &content, pager_blocks, list_menu_width);
}
