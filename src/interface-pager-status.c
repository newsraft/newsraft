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
	const struct render_block block = {wmessages, TEXT_PLAIN, NULL};
	const int pager_result = pager_view(&block, NULL, NULL);
	free_wstring(wmessages);
	return pager_result;
}
