#include "newsraft.h"

int
enter_status_pager_view_loop(void)
{
	struct string *messages = generate_string_with_status_messages_for_pager();
	if (messages == NULL) {
		return INPUT_ERROR;
	}
	if (messages->len == 0) {
		free_string(messages);
		return INPUT_ERROR;
	}
	struct wstring *wmessages = convert_string_to_wstring(messages);
	if (wmessages == NULL) {
		free_string(messages);
		return INPUT_ERROR;
	}
	free_string(messages);
	struct render_block block = {wmessages, TEXT_PLAIN, false};
	struct render_blocks_list blocks = {&block, 1, 0};
	if (start_pager_menu(&blocks) == false) {
		free_wstring(wmessages);
		return INPUT_ERROR;
	}
	enter_list_menu(PAGER_MENU, CFG_MENU_SECTION_ENTRY_FORMAT, true);
	input_cmd_id cmd;
	const struct wstring *macro;
	while (true) {
		cmd = get_input_cmd(NULL, &macro);
		if (handle_pager_menu_control(cmd) == true) {
			// Rest a little.
		} else if (cmd == INPUT_NAVIGATE_BACK || cmd == INPUT_QUIT_SOFT || cmd == INPUT_QUIT_HARD) {
			break;
		}
	}
	free_wstring(wmessages);
	return cmd;
}
