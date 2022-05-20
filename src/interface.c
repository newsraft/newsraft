#include "feedeater.h"

static bool
obtain_terminal_size(void)
{
	int terminal_width = getmaxx(stdscr);
	if (terminal_width == ERR) {
		FAIL("Failed to get width of the terminal!");
		return false;
	}
	if (terminal_width < 5) {
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

	list_menu_width = terminal_width;
	list_menu_height = terminal_height - 1; // Subtract 1 because we have status window.

	return true;
}

bool
curses_init(void)
{
	if (initscr() == NULL) {
		fprintf(stderr, "Initialization of curses data structures failed!\n");
		return false;
	}
	if (cbreak() == ERR) {
		fprintf(stderr, "Can not disable line buffering and erase/kill character-processing!\n");
		return false;
	}
	if (obtain_terminal_size() == false) {
		fprintf(stderr, "Invalid terminal size obtained!\n");
		return false;
	}
	if (curs_set(0) == ERR) {
		FAIL("Can not hide cursor!");
	}
	if (noecho() == ERR) {
		FAIL("Can not disable echoing of characters typed by the user!");
	}
	if (keypad(stdscr, TRUE) == ERR) { // used to enable arrow keys, function keys...
		FAIL("Can not enable extended keys!");
	}
	return true;
}

bool
resize_counter_action(void)
{
	if (obtain_terminal_size() == false) {
		// Some really crazy resize happend. It is either a glitch or user
		// deliberately trying to break something. This state is really
		// dangerous anyways.
		fprintf(stderr, "Don't flex around with me, okay?\n");
		return false;
	}

	adjust_list_menu();

	if (adjust_list_menu_format_buffer() == false) {
		return false;
	}

	status_resize();

	return true;
}
