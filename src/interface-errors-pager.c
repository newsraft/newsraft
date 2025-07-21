#include "newsraft.h"

struct menu_state *
errors_pager_loop(struct menu_state *m)
{
	if (m->feeds_original == NULL || m->feeds_count < 1) {
		return close_menu();
	}
	m->enumerator = &is_pager_pos_valid;
	m->printer    = &pager_menu_writer;

	struct render_blocks_list blocks = {NULL, 0, {}};
	pthread_mutex_lock(&interface_lock);
	for (size_t i = 0; i < m->feeds_count; ++i) {
		struct feed_entry *feed = m->feeds_original[i];
		if (feed->errors->len < 1) {
			continue;
		}
		blocks.ptr = newsraft_realloc(blocks.ptr, sizeof(struct render_block) * (blocks.len + 2));
		blocks.len += 2;
		struct string *header_str = blocks.len > 2 ? crtas("<br><hr><br>", 12) : crtes(100);
		catas(header_str, "<b>Errors of ", 13);
		catss(header_str, feed->url);
		catas(header_str, "</b><br><br>", 12);
		struct render_block *header_block = &blocks.ptr[blocks.len - 2];
		struct render_block *content_block = &blocks.ptr[blocks.len - 1];
		header_block->content = convert_string_to_wstring(header_str);
		header_block->content_type   = TEXT_HTML;
		header_block->needs_trimming = false;
		content_block->content = convert_string_to_wstring(feed->errors);
		content_block->content_type   = TEXT_RAW;
		content_block->needs_trimming = false;
		free_string(header_str);
	}
	pthread_mutex_unlock(&interface_lock);

	if (start_pager_menu(NULL, &blocks) == false) {
		free_render_blocks(&blocks);
		return close_menu();
	}

	start_menu();

	const struct wstring *arg;
	while (true) {
		input_id cmd = get_input(NULL, NULL, &arg);
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
