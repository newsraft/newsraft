#include "newsraft.h"

struct menu_state *
errors_pager_loop(struct menu_state *m)
{
	if (m->feeds_full == NULL || m->feeds_full_size < 1) {
		return menu_close();
	}
	m->enumerator = &is_pager_pos_valid;
	m->printer    = &pager_menu_writer;

	struct render_blocks_list blocks = {NULL, 0, {0}};
	pthread_mutex_lock(&interface_lock);
	for (size_t i = 0; i < m->feeds_full_size; ++i) {
		struct feed_entry *feed = m->feeds_full[i];
		if (feed->errors->len < 1) {
			continue;
		}
		struct string *header_str = blocks.len > 0 ? crtas("<br><hr><br>", 12) : crtes(100);
		catas(header_str, "<b>Errors of ", 13);
		catss(header_str, feed->url);
		catas(header_str, "</b><br><br>", 12);
		add_render_block(&blocks, header_str->ptr, header_str->len, TEXT_HTML, false);
		add_render_block(&blocks, feed->errors->ptr, feed->errors->len, TEXT_RAW, false);
		free_string(header_str);
	}
	pthread_mutex_unlock(&interface_lock);

	if (start_pager_menu(NULL, &blocks) == false) {
		free_render_blocks(&blocks);
		return menu_close();
	}

	menu_start(m);

	input_id cmd;
	const struct wstring *arg;
	while (menu_read(NULL, &cmd, NULL, &arg)) {
		if (handle_pager_menu_control(cmd)) {
			continue;
		} else if (cmd == INPUT_NAVIGATE_BACK || cmd == INPUT_QUIT_SOFT) {
			break;
		} else if (cmd == INPUT_QUIT_HARD) {
			free_render_blocks(&blocks);
			return NULL;
		}
	}

	free_render_blocks(&blocks);
	return menu_close();
}
