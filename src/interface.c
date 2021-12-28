#include "feedeater.h"

bool
obtain_terminal_size(void)
{
	list_menu_height = getmaxy(stdscr) - 1;
	if ((list_menu_height + 1) == ERR) {
		FAIL("Failed to get height of the terminal!");
		return false;
	}
	if (list_menu_height < 4) {
		FAIL("Terminal is too short!");
		return false;
	}

	list_menu_width = getmaxx(stdscr);
	if (list_menu_width == ERR) {
		FAIL("Failed to get width of the terminal!");
		return false;
	}
	if (list_menu_width < 4) {
		FAIL("Terminal is too narrow!");
		return false;
	}

	return true;
}

int
curses_init(void)
{
	if (initscr() == NULL) {
		fprintf(stderr, "Initialization of curses data structures failed!\n");
		return 1;
	}
	if (cbreak() == ERR) {
		fprintf(stderr, "Can not disable line buffering and erase/kill character-processing!\n");
		return 1;
	}
	if (obtain_terminal_size() == false) {
		fprintf(stderr, "Invalid terminal size obtained!\n");
		return 1;
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
	return 0;
}
