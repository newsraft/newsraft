#include "feedeater.h"

static WINDOW *input_win = NULL;

int
input_create(void)
{
	input_win = newwin(1, 1, LINES, COLS);
	if (input_win == NULL) {
		fprintf(stderr, "could not create input field\n");
		return 1;
	}

	if (cbreak() == ERR) {
		fprintf(stderr, "can't disable line buffering and erase/kill character-processing\n");
		delwin(input_win);
		return 1;
	} 
	if (curs_set(0) == ERR) { // try to hide cursor
		debug_write(DBG_WARN, "can't hide cursor\n");
	}
	if (noecho() == ERR) {
		debug_write(DBG_WARN, "can't disable echoing of characters typed by the user\n");
	}
	if (keypad(input_win, TRUE) == ERR) { // used to enable arrow keys, function keys...
		debug_write(DBG_WARN, "can't enable extended keys\n");
	}

	return 0;
}

int
input_wgetch(void)
{
	return wgetch(input_win);
}


void
input_delete(void)
{
	delwin(input_win);
}
