#include "feedeater.h"

int
curses_init(void)
{
	if (initscr() == NULL) {
		fprintf(stderr, "Initialization of curses data structures faile!\n");
		return 1; // failure
	}
	if (cbreak() == ERR) {
		fprintf(stderr, "Can not disable line buffering and erase/kill character-processing!\n");
		return 1; // failure
	}
	if (curs_set(0) == ERR) { // try to hide cursor
		debug_write(DBG_FAIL, "Can not hide cursor!\n");
	}
	if (noecho() == ERR) {
		debug_write(DBG_FAIL, "Can not disable echoing of characters typed by the user!\n");
	}
	if (keypad(stdscr, TRUE) == ERR) { // used to enable arrow keys, function keys...
		debug_write(DBG_FAIL, "Can not enable extended keys!\n");
	}
	return 0; // success
}
