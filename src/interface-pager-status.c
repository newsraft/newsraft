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
	struct render_block block = {wmessages, TEXT_PLAIN, 0};
	struct render_blocks_list blocks = {&block, 1, NULL, 0};
	const int pager_result = pager_view(&blocks, NULL, NULL);
	free_wstring(wmessages);
	return pager_result;
}
