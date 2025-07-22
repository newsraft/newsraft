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
	wmove(status_window, 0);

	if (they_want_us_to_stop == true) {
		waddwstr(status_window, L"Terminating...");
		wbkgd(status_window, get_cfg_color(NULL, CFG_COLOR_STATUS_FAIL));
	} else if (search_mode_is_enabled == true) {
		waddwstr(status_window, L"/");
		waddwstr(status_window, search_mode_text_input->ptr);
		wbkgd(status_window, get_cfg_color(NULL, CFG_COLOR_STATUS));
	} else if (!STRING_IS_EMPTY(message_text)) {
		waddstr(status_window, message_text->ptr);
		wbkgd(status_window, get_cfg_color(NULL, message_color));
	} else {
		struct string *path = NULL;
		if (get_cfg_bool(NULL, CFG_STATUS_SHOW_MENU_PATH)) {
			path = crtes(100);
			write_menu_path_string(path, NULL);
		}
		if (path != NULL && path->len > 0) {
			waddstr(status_window, path->ptr);
		} else {
			waddwstr(status_window, get_cfg_wstring(NULL, CFG_STATUS_PLACEHOLDER)->ptr);
		}
		wbkgd(status_window, get_cfg_color(NULL, CFG_COLOR_STATUS));
		free_string(path);
	}

	// Print count value
	wmove(status_window, list_menu_width - 11);
	if (count_buf_len > 0) {
		waddwstr(status_window, L"  "); // Little divider to separate from previous stuff
		waddstr(status_window, count_buf);
	}

	tb_present();
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
	status_window = newwin(list_menu_height);
	INFO("Created status window");
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
	static char key[1000];
	static size_t queued_action_index = 0;
	if (queued_action_index > 0) {
		input_id next = get_action_of_bind(ctx, key, queued_action_index, p_arg);
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
		int status = get_wch(key);
		pthread_mutex_unlock(&interface_lock);
		if (status == TB_ERR) {
			if (they_want_us_to_break_input == true) {
				they_want_us_to_break_input = false;
				return INPUT_ERROR;
			}
			nanosleep(&input_polling_period, NULL);
			continue;
		} else if (status == TB_EVENT_RESIZE) {
			return resize_handler();
		}
		INFO("Read key: %s", key);
		if (search_mode_is_enabled == true) {
			if (strcmp(key, "enter") == 0 || strcmp(key, "escape") == 0) {
				search_mode_is_enabled = false;
				status_clean();
				if (strcmp(key, "enter") == 0) return INPUT_APPLY_SEARCH_MODE_FILTER;
			} else if (strcmp(key, "backspace") == 0) {
				if (search_mode_text_input->len > 0) {
					search_mode_text_input->len -= 1;
					search_mode_text_input->ptr[search_mode_text_input->len] = L'\0';
					update_status_window_content();
				} else {
					search_mode_is_enabled = false;
					status_clean();
				}
			} else {
				struct wstring *wkey = convert_array_to_wstring(key, strlen(key));
				wcatss(search_mode_text_input, wkey);
				free_wstring(wkey);
				update_status_window_content();
			}
		} else if (ISDIGIT(key[0])) {
			count_buf_len %= 9;
			count_buf[count_buf_len++] = key[0];
			count_buf[count_buf_len] = '\0';
			update_status_window_content();
		} else {
			input_id cmd = get_action_of_bind(ctx, key, 0, p_arg);
			uint32_t count_value = 1;
			if (sscanf(count_buf, "%" SCNu32, &count_value) != 1) {
				count_value = 1;
			}
			count_buf[0] = '\0';
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
