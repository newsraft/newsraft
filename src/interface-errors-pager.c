#include "newsraft.h"

struct menu_state *
errors_pager_loop(struct menu_state *m)
{
	if (m->feeds_original == NULL || m->feeds_count != 1) {
		return close_menu();
	}
	m->enumerator   = &is_pager_pos_valid;
	m->write_action = &pager_menu_writer;
	struct render_block *block = calloc(1, sizeof(struct render_block));
	if (block == NULL) {
		return close_menu();
	}
	block->content        = convert_string_to_wstring(m->feeds_original[0]->errors);
	block->content_type   = TEXT_RAW;
	block->needs_trimming = false;
	struct render_blocks_list blocks = {block, 1, {}};
	if (start_pager_menu(NULL, &blocks) == false) {
		free_render_blocks(&blocks);
		return close_menu();
	}
	start_menu();
	const struct wstring *macro;
	while (true) {
		input_id cmd = get_input(NULL, NULL, &macro);
		if (handle_pager_menu_control(cmd) == true) {
			// Rest a little.
		} else if (cmd == INPUT_NAVIGATE_BACK || cmd == INPUT_QUIT_SOFT) {
			break;
		} else if (cmd == INPUT_QUIT_HARD) {
			free_render_blocks(&blocks);
			return NULL;
		}
	}
	free_render_blocks(&blocks);
	return close_menu();
}
