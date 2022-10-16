#include <stdlib.h>
#include <curses.h>
#include "newsraft.h"

static WINDOW *counter_window = NULL;

// We take 999999999 as the maximum value for count variable to avoid overflow
// of the uint32_t integer. The width of this number is hardcoded into the
// terminal width limit, so when changing it here, consider changing the limit.
// 9 (max length of input) + 1 (terminator) = 10
static char count_buf[10];
static uint8_t count_buf_len = 0;

static inline void
counter_update(void)
{
	if (counter_window != NULL) {
		werase(counter_window);
		if (count_buf_len != 0) {
			mvwaddnstr(counter_window, 0, 0, count_buf, count_buf_len);
		}
		wrefresh(counter_window);
	}
}

bool
counter_recreate(void)
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
	if (keypad(counter_window, TRUE) == ERR) {
		WARN("Can't enable keypad and function keys reading support for counter window!");
	}
	counter_update();
	return true;
}

void
counter_send_character(char c)
{
	if (count_buf_len == 9) {
		count_buf_len = 0;
	}
	count_buf[count_buf_len++] = c;
	counter_update();
}

uint32_t
counter_extract_count(void)
{
	uint32_t count;
	if (count_buf_len == 0) {
		count = 1;
	} else {
		count_buf[count_buf_len] = '\0';
		if (sscanf(count_buf, "%" SCNu32, &count) != 1) {
			count = 1;
		}
	}
	return count;
}

void
counter_clean(void)
{
	if (counter_window != NULL) {
		werase(counter_window);
		wrefresh(counter_window);
	}
	count_buf_len = 0;
}

int
read_key_from_counter_window(void)
{
	// We can't read keys from stdscr via getch() function because calling it
	// will bring stdscr on top of other windows and overlap them.
	int c = wgetch(counter_window);
	INFO("Received \"%c\" character with %d key code.", c, c);
	return c;
}

void
counter_delete(void)
{
	INFO("Freeing counter window.");
	if (counter_window != NULL) {
		delwin(counter_window);
	}
}
