#include <stdlib.h>
#include "newsraft.h"

struct status_message {
	struct string *text;
	config_entry_id color;
};

static WINDOW *status_window = NULL;
static bool status_window_is_clean = true;
static volatile bool status_window_is_cleanable = true;
static struct status_message *messages;
static size_t messages_len = 0;
static size_t messages_lim = 0;

// We take 999999999 as the maximum value for count variable to avoid overflow
// of the uint32_t integer. The width of this number is hardcoded into the
// terminal width limit, so when changing it here, consider changing the limit.
// 9 (max length of input) + 1 (terminator) = 10
static char count_buf[10];
static uint8_t count_buf_len = 0;

static const struct timespec input_polling_period = {0, 30000000}; // 0.03 seconds
static volatile bool they_want_us_to_break_input = false;

static inline void
update_status_window_content_unprotected(void)
{
	werase(status_window);
	wmove(status_window, 0, 0);

	if (they_want_us_to_terminate == true) {
		wattrset(status_window, get_cfg_color(NULL, CFG_COLOR_STATUS_FAIL));
		waddnstr(status_window, "Terminating...", list_menu_width);
	} else if (search_mode_is_enabled == true) {
		wattrset(status_window, A_NORMAL);
		waddwstr(status_window, L"/");
		waddwstr(status_window, search_mode_text_input->ptr);
	} else if (status_window_is_clean == false) {
		wattrset(status_window, get_cfg_color(NULL, messages[(messages_len - 1) % messages_lim].color));
		waddnstr(status_window, messages[(messages_len - 1) % messages_lim].text->ptr, list_menu_width);
	}

	// Print counter value
	wmove(status_window, 0, list_menu_width - 9);
	wattrset(status_window, A_NORMAL);
	if (count_buf_len != 0) {
		waddnstr(status_window, count_buf, count_buf_len);
	}

	wrefresh(status_window);
}

void
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

	update_status_window_content_unprotected();
	return true;
}

bool
allocate_status_messages_buffer(void)
{
	messages_lim = get_cfg_uint(NULL, CFG_STATUS_MESSAGES_COUNT_LIMIT);
	messages_lim |= 1; // Make sure it's not a zero.
	messages = calloc(messages_lim, sizeof(struct status_message));
	if (messages == NULL) {
		goto error;
	}
	for (size_t i = 0; i < messages_lim; ++i) {
		messages[i].text = crtes(100);
		if (messages[i].text == NULL) {
			goto error; // Since we calloced messages buffer, everything after that remains NULL.
		}
	}
	return true;
error:
	write_error("Not enough memory for status messages buffer!\n");
	return false;
}

void
status_clean_unprotected(void)
{
	if (status_window_is_cleanable == true) {
		status_window_is_clean = true;
		if (search_mode_is_enabled == false) {
			werase(status_window);
			wrefresh(status_window);
		}
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
	pthread_mutex_lock(&interface_lock);
	va_list args;
	va_start(args, format);
	if (string_vprintf(messages[messages_len % messages_lim].text, format, args) == false) {
		goto undo;
	}
	messages[messages_len % messages_lim].color = color;
	INFO("Printed status message: %s", messages[messages_len % messages_lim].text->ptr);
	messages_len += 1;
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
	size_t i = (messages_len - 1) % messages_lim;
	for (size_t j = MIN(messages_len, messages_lim); j > 0; --j) {
		catss(str, messages[i].text);
		catcs(str, '\n');
		if (i == 0) {
			i = messages_lim - 1;
		} else {
			i -= 1;
		}
	}
	return str;
}

void
status_delete(void)
{
	INFO("Freeing status field resources.");
	for (size_t i = 0; i < messages_lim; ++i) {
		free_string(messages[i].text);
	}
	free(messages);
	delwin(status_window);
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
	while (they_want_us_to_terminate == false) {
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
			uint32_t counter_value = 1;
			count_buf[count_buf_len] = '\0';
			if (sscanf(count_buf, "%" SCNu32, &counter_value) != 1) {
				counter_value = 1;
			}
			count_buf_len = 0;
			if (cmd == INPUT_START_SEARCH_INPUT) {
				wstr_set(&search_mode_text_input, L"", 0, 100);
				search_mode_is_enabled = true;
				update_status_window_content();
			} else {
				if (count) {
					*count = counter_value;
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
