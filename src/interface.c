#include <stdlib.h>
#include "newsraft.h"

bool search_mode_is_enabled = false;
struct wstring *search_mode_text_input = NULL;
static bool paint_it_black = true;

pthread_mutex_t interface_lock = PTHREAD_MUTEX_INITIALIZER;

static bool
obtain_list_menu_size(size_t *width, size_t *height)
{
	int terminal_width = getmaxx(stdscr);
	if (terminal_width == ERR) {
		FAIL("Failed to get width of the terminal!");
		return false;
	}
	// This is really critical! You will get integer overflow if terminal_width
	// is less than 12. You have been warned.
	if (terminal_width < 12) {
		FAIL("Terminal is too narrow!");
		return false;
	}
	int terminal_height = getmaxy(stdscr);
	if (terminal_height == ERR) {
		FAIL("Failed to get height of the terminal!");
		return false;
	}
	if (terminal_height < 5) {
		FAIL("Terminal is too short!");
		return false;
	}

	INFO("Obtained terminal size: %d width, %d height.", terminal_width, terminal_height);

	*width = terminal_width;
	*height = terminal_height - 1; // Subtract 1 because we have status window.

	return true;
}

bool
curses_init(void)
{
	if (initscr() == NULL) {
		write_error("Initialization of curses data structures failed!\n");
		return false;
	}
	if (cbreak() == ERR) {
		write_error("Can't disable line buffering and erase/kill characters processing!\n");
		return false;
	}
	if (obtain_list_menu_size(&list_menu_width, &list_menu_height) == false) {
		write_error("Invalid terminal size obtained!\n");
		return false;
	}
	if (curs_set(0) == ERR) {
		WARN("Can't hide cursor!");
	}
	if (noecho() == ERR) {
		WARN("Can't disable echoing of characters typed by the user!");
	}
	if (getenv("NO_COLOR") != NULL) {
		INFO("NO_COLOR environment variable is set, canceling colors initialization.");
	} else if (has_colors() == FALSE) {
		WARN("Terminal emulator doesn't support colors!");
	} else if (use_default_colors() == ERR) {
		WARN("Can't obtain default terminal colors!");
	} else if (start_color() == ERR) {
		WARN("Initialization of curses color structures failed!");
	} else {
		paint_it_black = false; // Some iridescent sensation at last!
		INFO("Maximum number of colors: %d", COLORS);
		INFO("Maximum number of color pairs: %d", COLOR_PAIRS);
	}
	INFO("The value of KEY_RESIZE code is %d.", KEY_RESIZE);
	return true;
}

input_id
resize_handler(void)
{
	pthread_mutex_lock(&interface_lock);

	// We need to call clear and refresh before all resize actions because
	// the junk text of previous size may remain in inactive areas.
	clear();
	refresh();

	if (obtain_list_menu_size(&list_menu_width, &list_menu_height) == false) {
		// Some really crazy resize happend. It is either a glitch or user
		// deliberately trying to break something. This state is unusable anyways.
		write_error("Don't flex around with me, okay?\n");
		goto error;
	}
	if (adjust_list_menu() == false) {
		goto error;
	}
	if (status_recreate_unprotected() == false) {
		goto error;
	}
	if (is_current_menu_a_pager() == true) {
		refresh_pager_menu();
	}
	redraw_list_menu_unprotected();
	pthread_mutex_unlock(&interface_lock);
	return INPUT_ERROR;
error:
	tell_program_to_terminate_safely_and_quickly(0);
	pthread_mutex_unlock(&interface_lock);
	return INPUT_QUIT_HARD;
}

bool
call_resize_handler_if_current_list_menu_size_is_different_from_actual(void)
{
	size_t width = 0, height = 0;
	obtain_list_menu_size(&width, &height);
	if (width != list_menu_width || height != list_menu_height) {
		resize_handler();
		return true;
	}
	return false;
}

bool
arent_we_colorful(void)
{
	return !paint_it_black;
}
