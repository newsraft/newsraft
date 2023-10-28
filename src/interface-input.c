#include <stdlib.h>
#include "newsraft.h"

static WINDOW *counter_window = NULL;

// We take 999999999 as the maximum value for count variable to avoid overflow
// of the uint32_t integer. The width of this number is hardcoded into the
// terminal width limit, so when changing it here, consider changing the limit.
// 9 (max length of input) + 1 (terminator) = 10
static char count_buf[10];
static uint8_t count_buf_len = 0;
static const struct timespec input_polling_period = {0, 30000000}; // 0.03 seconds

static volatile bool they_want_us_to_break_input = false;

static inline void
counter_update_unprotected(void)
{
	werase(counter_window);
	if (count_buf_len != 0) {
		mvwaddnstr(counter_window, 0, 0, count_buf, count_buf_len);
	}
	wrefresh(counter_window);
}

bool
counter_recreate_unprotected(void)
{
	if (counter_window != NULL) {
		delwin(counter_window);
	}
	counter_window = newwin(1, 9, list_menu_height, list_menu_width - 9);
	if (counter_window == NULL) {
		fputs("Failed to create counter window!\n", stderr);
		return false;
	}
	INFO("Created counter window.");
	nodelay(counter_window, TRUE);
	if (keypad(counter_window, TRUE) == ERR) {
		WARN("Can't enable keypad and function keys reading support for counter window!");
	}
	counter_update_unprotected();
	return true;
}

void
tell_program_to_terminate_safely_and_quickly(int dummy)
{
	(void)dummy;
	they_want_us_to_terminate = true;
}

input_cmd_id
get_input_command(uint32_t *count, const struct wstring **macro_ptr)
{
	static int c = 0;
	static size_t queued_action_index = 0;
	if (queued_action_index > 0) {
		input_cmd_id next = get_action_of_bind(c, queued_action_index, macro_ptr);
		if (next != INPUT_ERROR) {
			queued_action_index += 1;
			return next;
		} else {
			queued_action_index = 0;
		}
	}
	while (they_want_us_to_terminate == false) {
		// We can't read keys from stdscr via getch() function
		// because calling it will bring stdscr on top of other
		// windows and overlap them.
		pthread_mutex_lock(&interface_lock);
		c = wgetch(counter_window);
		pthread_mutex_unlock(&interface_lock);
		if (c == ERR) {
			if (they_want_us_to_break_input == true) {
				they_want_us_to_break_input = false;
				return INPUT_ERROR;
			}
			nanosleep(&input_polling_period, NULL);
			continue;
		} else if (c == KEY_RESIZE) {
			return resize_handler();
		}
		INFO("Received \"%c\" character with %d key code.", c, c);
		if (search_mode_is_enabled == true) {
			if (c == '\n' || c == 27) {
				search_mode_is_enabled = false;
				status_clean();
				if (c == '\n') return INPUT_APPLY_SEARCH_MODE_FILTER;
			} else if (c == KEY_BACKSPACE || c == KEY_DC || c == 127) {
				if (search_mode_text_input->len > 0) {
					search_mode_text_input->len -= 1;
					search_mode_text_input->ptr[search_mode_text_input->len] = '\0';
					update_status_window_content();
				} else {
					search_mode_is_enabled = false;
					status_clean();
				}
			} else {
				catcs(search_mode_text_input, c);
				update_status_window_content();
			}
		} else if (ISDIGIT(c)) {
			count_buf_len %= 9;
			count_buf[count_buf_len++] = c;
			pthread_mutex_lock(&interface_lock);
			counter_update_unprotected();
			pthread_mutex_unlock(&interface_lock);
		} else {
			input_cmd_id cmd = get_action_of_bind(c, 0, macro_ptr);
			if (cmd == INPUT_START_SEARCH_INPUT) {
				cpyas(&search_mode_text_input, "", 0);
				search_mode_is_enabled = true;
				update_status_window_content();
			} else {
				count_buf[count_buf_len] = '\0';
				if (count != NULL && sscanf(count_buf, "%" SCNu32, count) != 1) {
					*count = 1;
				}
				count_buf_len = 0;
				pthread_mutex_lock(&interface_lock);
				counter_update_unprotected();
				pthread_mutex_unlock(&interface_lock);
				queued_action_index = 1;
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

void
counter_delete(void)
{
	if (counter_window != NULL) {
		INFO("Freeing counter window.");
		delwin(counter_window);
	}
}
