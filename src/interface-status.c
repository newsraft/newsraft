#include <stdlib.h>
#include "newsraft.h"

static WINDOW *status_window = NULL;
static volatile bool status_window_is_cleanable = true;
static volatile bool status_window_is_initialized = false;
static struct string *message_text = NULL;
static config_entry_id message_color;
static bool search_mode_is_enabled = false;
static struct wstring *search_mode_text_input = NULL;

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
	} else if (!STRING_IS_EMPTY(message_text)) {
		wbkgd(status_window, get_cfg_color(NULL, message_color));
		waddnstr(status_window, message_text->ptr, list_menu_width);
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
	if (status_window_is_cleanable == true && !STRING_IS_EMPTY(message_text)) {
		empty_string(message_text);
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
	if (status_window_is_initialized != true) {
		va_list args;
		va_start(args, format);
		vprintf(format, args);
		putchar('\n');
		fflush(stdout);
		va_end(args);
		return;
	}

	pthread_mutex_lock(&interface_lock);
	struct string *new_message = crtes(100);
	va_list args;
	va_start(args, format);
	str_vappendf(new_message, format, args);
	va_end(args);
	message_color = color;
	cpyss(&message_text, new_message);
	free_string(new_message);
	INFO("Printed status message: %s", message_text->ptr);
	update_status_window_content_unprotected();
	pthread_mutex_unlock(&interface_lock);
}

void
status_delete(void)
{
	pthread_mutex_lock(&interface_lock);
	free_string(message_text);
	message_text = NULL;
	delwin(status_window);
	pthread_mutex_unlock(&interface_lock);
}

input_id
get_input(struct input_binding *ctx, uint32_t *count, const struct wstring **p_arg)
{
	static wint_t c = 0;
	static size_t queued_action_index = 0;
	if (queued_action_index > 0) {
		input_id next = get_action_of_bind(ctx, keyname(c), queued_action_index, p_arg);
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
			input_id cmd = get_action_of_bind(ctx, key, 0, p_arg);
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

struct string *
pop_search_filter(void)
{
	if (search_mode_is_enabled == true) {
		return NULL; // Must not yield search filter when it's not complete yet!
	}
	if (search_mode_text_input != NULL && search_mode_text_input->len > 0) {
		struct string *search_query = convert_wstring_to_string(search_mode_text_input);
		empty_wstring(search_mode_text_input);
		return search_query;
	}
	return NULL;
}
