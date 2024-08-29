#include <stdlib.h>
#include "newsraft.h"

struct status_message {
	struct string *text;
	config_entry_id color;
	struct status_message *next;
	struct status_message *prev;
};

static WINDOW *status_window = NULL;
static bool status_window_is_clean = true;
static volatile bool status_window_is_cleanable = true;
static volatile bool status_window_is_initialized = false;
static struct status_message *messages; // Cycled list
static volatile size_t messages_len = 0;

// We take 999999999 as the maximum value for count variable to avoid overflow
// of the uint32_t integer. The width of this number is hardcoded into the
// terminal width limit, so when changing it here, consider changing the limit.
// 9 (max length of input) + 1 (terminator) = 10
static char count_buf[10];
static uint8_t count_buf_len = 0;

static const struct timespec input_polling_period = {0, 30000000}; // 0.03 seconds
static volatile bool they_want_us_to_break_input = false;

void
update_status_window_content_unprotected(void)
{
	if (status_window_is_initialized != true) {
		return;
	}
	// FIXME: werase doesn't clear screen garbage, so sometimes artifacts
	// appear in the window. The solution to this is wclear, but it makes
	// screen flicker on the old hardware.
	werase(status_window);
	wmove(status_window, 0, 0);

	if (they_want_us_to_stop == true) {
		wbkgd(status_window, get_cfg_color(NULL, CFG_COLOR_STATUS_FAIL));
		waddnstr(status_window, "Terminating...", list_menu_width);
	} else if (search_mode_is_enabled == true) {
		wbkgd(status_window, get_cfg_color(NULL, CFG_COLOR_STATUS));
		waddwstr(status_window, L"/");
		waddwstr(status_window, search_mode_text_input->ptr);
	} else if (status_window_is_clean == false) {
		if (messages != NULL) {
			wbkgd(status_window, get_cfg_color(NULL, messages->color));
			waddnstr(status_window, messages->text->ptr, list_menu_width);
		}
	} else {
		wbkgd(status_window, get_cfg_color(NULL, CFG_COLOR_STATUS));
		struct string *path = NULL;
		if (get_cfg_bool(NULL, CFG_STATUS_SHOW_MENU_PATH)) {
			path = crtes(100);
			write_menu_path_string(path, NULL);
		}
		if (path != NULL && path->len > 0) {
			waddnstr(status_window, path->ptr, list_menu_width);
		} else {
			waddnstr(status_window, get_cfg_string(NULL, CFG_STATUS_PLACEHOLDER)->ptr, list_menu_width);
		}
		free_string(path);
	}

	// Print count value
	wmove(status_window, 0, list_menu_width - 11);
	if (count_buf_len > 0) {
		waddstr(status_window, "  "); // Little divider to separate from previous stuff
		waddnstr(status_window, count_buf, count_buf_len);
	}

	wrefresh(status_window);
}

static void
update_status_window_content(void)
{
	pthread_mutex_lock(&interface_lock);
	update_status_window_content_unprotected();
	pthread_mutex_unlock(&interface_lock);
}

bool
status_recreate_unprotected(void)
{
	if (status_window != NULL) {
		delwin(status_window);
	}

	status_window = newwin(1, list_menu_width, list_menu_height, 0);
	if (status_window == NULL) {
		write_error("Failed to create status window!\n");
		return false;
	}

	INFO("Created status window");

	nodelay(status_window, TRUE);
	if (keypad(status_window, TRUE) == ERR) {
		WARN("Can't enable keypad and function keys reading support for status window!");
	}

	status_window_is_initialized = true;
	update_status_window_content_unprotected();
	return true;
}

void
status_clean_unprotected(void)
{
	if (status_window_is_cleanable == true) {
		status_window_is_clean = true;
		update_status_window_content_unprotected();
	}
}

void
status_clean(void)
{
	pthread_mutex_lock(&interface_lock);
	status_clean_unprotected();
	pthread_mutex_unlock(&interface_lock);
}

void
prevent_status_cleaning(void)
{
	status_window_is_cleanable = false;
}

void
allow_status_cleaning(void)
{
	status_window_is_cleanable = true;
}

void
status_write(config_entry_id color, const char *format, ...)
{
	size_t limit = get_cfg_uint(NULL, CFG_STATUS_MESSAGES_COUNT_LIMIT);
	if (limit == 0) {
		return;
	}

	pthread_mutex_lock(&interface_lock);
	if (messages_len < limit) {
		struct status_message *msg = calloc(1, sizeof(struct status_message));
		if (msg == NULL) {
			pthread_mutex_unlock(&interface_lock);
			return;
		}
		msg->text = crtes(1);
		if (msg->text == NULL) {
			free(msg);
			pthread_mutex_unlock(&interface_lock);
			return;
		}
		if (messages == NULL) {
			msg->next = msg;
			msg->prev = msg;
		} else {
			msg->next = messages;
			msg->prev = messages->prev;
			messages->prev->next = msg;
			messages->prev = msg;
		}
		messages = msg;
		messages_len += 1;
	} else {
		messages = messages->prev;
	}
	va_list args;
	va_start(args, format);
	if (string_vprintf(messages->text, format, args) == false) {
		goto undo;
	}
	messages->color = color;
	INFO("Printed status message: %s", messages->text->ptr);
	status_window_is_clean = false;
	update_status_window_content_unprotected();
undo:
	va_end(args);
	pthread_mutex_unlock(&interface_lock);
}

struct string *
generate_string_with_status_messages_for_pager(void)
{
	struct string *str = crtes(1000);
	if (str == NULL) {
		FAIL("Not enough memory for string with status messages for pager!");
		return NULL;
	}
	pthread_mutex_lock(&interface_lock);
	if (messages_len > 0) {
		struct status_message *m = messages;
		do {
			catss(str, m->text);
			catcs(str, '\n');
			m = m->next;
		} while (m != messages);
	}
	pthread_mutex_unlock(&interface_lock);
	return str;
}

void
status_delete(void)
{
	pthread_mutex_lock(&interface_lock);
	if (messages_len > 0) {
		struct status_message *m = messages;
		do {
			free_string(m->text);
			struct status_message *tmp = m;
			m = m->next;
			free(tmp);
		} while (m != messages);
	}
	messages = NULL;
	delwin(status_window);
	pthread_mutex_unlock(&interface_lock);
}

input_id
get_input(struct input_binding *ctx, uint32_t *count, const struct wstring **macro_ptr)
{
	static wint_t c = 0;
	static size_t queued_action_index = 0;
	if (queued_action_index > 0) {
		input_id next = get_action_of_bind(ctx, keyname(c), queued_action_index, macro_ptr);
		if (next != INPUT_ERROR) {
			queued_action_index += 1;
			return next;
		} else {
			queued_action_index = 0;
		}
	}
	while (they_want_us_to_stop == false) {
		// We have to read input from a child window because
		// reading it from the main window will bring stdscr
		// on top of other windows and overlap them.
		pthread_mutex_lock(&interface_lock);
		int status = wget_wch(status_window, &c);
		pthread_mutex_unlock(&interface_lock);
		if (status == ERR) {
			if (they_want_us_to_break_input == true) {
				they_want_us_to_break_input = false;
				return INPUT_ERROR;
			}
			nanosleep(&input_polling_period, NULL);
			continue;
		} else if (c == KEY_RESIZE) {
			return resize_handler();
		}
		const char *key = keyname(c);
		INFO("Read key %d (\'%lc\', \"%s\")", c, c, key ? key : "ERROR");
		if (search_mode_is_enabled == true) {
			if (c == '\n' || c == 27) {
				search_mode_is_enabled = false;
				status_clean();
				if (c == '\n') return INPUT_APPLY_SEARCH_MODE_FILTER;
			} else if (c == KEY_BACKSPACE || c == KEY_DC || c == 127) {
				if (search_mode_text_input->len > 0) {
					search_mode_text_input->len -= 1;
					search_mode_text_input->ptr[search_mode_text_input->len] = L'\0';
					update_status_window_content();
				} else {
					search_mode_is_enabled = false;
					status_clean();
				}
			} else {
				wcatcs(search_mode_text_input, c);
				update_status_window_content();
			}
		} else if (ISDIGIT(c)) {
			count_buf_len %= 9;
			count_buf[count_buf_len++] = c;
			update_status_window_content();
		} else {
			input_id cmd = get_action_of_bind(ctx, key, 0, macro_ptr);
			uint32_t count_value = 1;
			count_buf[count_buf_len] = '\0';
			if (sscanf(count_buf, "%" SCNu32, &count_value) != 1) {
				count_value = 1;
			}
			count_buf_len = 0;
			if (cmd == INPUT_START_SEARCH_INPUT) {
				wstr_set(&search_mode_text_input, L"", 0, 100);
				search_mode_is_enabled = true;
				update_status_window_content();
			} else {
				if (count) {
					*count = count_value;
				}
				queued_action_index = 1;
				update_status_window_content();
				return cmd;
			}
		}
	}
	INFO("Received signal requesting termination of program.");
	return INPUT_QUIT_HARD;
}

void
break_getting_input_command(void)
{
	they_want_us_to_break_input = true;
}
