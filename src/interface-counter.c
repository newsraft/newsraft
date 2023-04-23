#include <stdlib.h>
#include <ctype.h>
#include "newsraft.h"

static WINDOW *counter_window = NULL;

// We take 999999999 as the maximum value for count variable to avoid overflow
// of the uint32_t integer. The width of this number is hardcoded into the
// terminal width limit, so when changing it here, consider changing the limit.
// 9 (max length of input) + 1 (terminator) = 10
static char count_buf[10];
static uint8_t count_buf_len = 0;
static const struct timespec input_polling_period = {0, 4000000}; // 0.004 seconds

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

int
read_counted_key_from_counter_window(uint32_t *count)
{
	int c;
	while (true) {
		while (true) {
			// We can't read keys from stdscr via getch() function
			// because calling it will bring stdscr on top of other
			// windows and overlap them.
			pthread_mutex_lock(&interface_lock);
			c = wgetch(counter_window);
			pthread_mutex_unlock(&interface_lock);
			if (c == ERR) {
				nanosleep(&input_polling_period, NULL);
			} else {
				break;
			}
		}
		INFO("Received \"%c\" character with %d key code.", c, c);
		if (c == KEY_RESIZE) {
			return c;
		} else if (isdigit(c) == 0) {
			count_buf[count_buf_len] = '\0';
			if (sscanf(count_buf, "%" SCNu32, count) != 1) {
				*count = 1;
			}
			count_buf_len = 0;
			pthread_mutex_lock(&interface_lock);
			counter_update_unprotected();
			pthread_mutex_unlock(&interface_lock);
			return c;
		}
		count_buf_len %= 9;
		count_buf[count_buf_len++] = c;
		pthread_mutex_lock(&interface_lock);
		counter_update_unprotected();
		pthread_mutex_unlock(&interface_lock);
	}
}

void
counter_delete(void)
{
	if (counter_window != NULL) {
		INFO("Freeing counter window.");
		delwin(counter_window);
	}
}
