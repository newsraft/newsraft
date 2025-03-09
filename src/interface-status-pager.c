#include "newsraft.h"

struct menu_state *
status_pager_loop(struct menu_state *dest)
{
	dest->enumerator   = &is_pager_pos_valid;
	dest->write_action = &pager_menu_writer;
	struct string *messages = generate_string_with_status_messages_for_pager();
	if (STRING_IS_EMPTY(messages)) {
		free_string(messages);
		return close_menu();
	}
	struct wstring *wmessages = convert_string_to_wstring(messages);
	if (wmessages == NULL) {
		free_string(messages);
		return close_menu();
	}
	free_string(messages);
	struct render_block *block = calloc(1, sizeof(struct render_block));
	if (block == NULL) {
		free_wstring(wmessages);
		return close_menu();
	}
	block->content        = wmessages;
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
