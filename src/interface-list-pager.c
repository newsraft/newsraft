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
		for (size_t i = 0; i < content.lines[index].ws->len; ++i) {
			wattrset(w, content.lines[index].hints[i]);
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
	free_render_result(&content);
	return render_data(pager_context, &content, pager_blocks, list_menu_width);
}
