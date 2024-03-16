#include "newsraft.h"

struct menu_state *
status_pager_loop(struct menu_state *dest)
{
	dest->enumerator   = &is_pager_pos_valid;
	dest->write_action = &pager_menu_writer;
	struct string *messages = generate_string_with_status_messages_for_pager();
	if (messages == NULL || messages->len == 0) {
		free_string(messages);
		return setup_menu(NULL, NULL, 0, 0);
	}
	struct wstring *wmessages = convert_string_to_wstring(messages);
	if (wmessages == NULL) {
		free_string(messages);
		return setup_menu(NULL, NULL, 0, 0);
	}
	free_string(messages);
	struct render_block block = {wmessages, TEXT_PLAIN, false};
	struct render_blocks_list blocks = {&block, 1, 0};
	if (start_pager_menu(&blocks) == false) {
		free_wstring(wmessages);
		return setup_menu(NULL, NULL, 0, 0);
	}
	start_menu();
	const struct wstring *macro;
	for (input_cmd_id cmd = get_input_cmd(NULL, &macro) ;; cmd = get_input_cmd(NULL, &macro)) {
		if (handle_pager_menu_control(cmd) == true) {
			// Rest a little.
		} else if (cmd == INPUT_NAVIGATE_BACK || cmd == INPUT_QUIT_SOFT) {
			break;
		} else if (cmd == INPUT_QUIT_HARD) {
			free_wstring(wmessages);
			return NULL;
		}
	}
	free_wstring(wmessages);
	return setup_menu(NULL, NULL, 0, 0);
}
