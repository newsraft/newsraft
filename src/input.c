#include "feedeater.h"

WINDOW *input_win = NULL;

int
input_create(void)
{
	input_win = newwin(1, 1, LINES, COLS);
	if (input_win == NULL) {
		return 0;
	}

	// used to recognize arrow and numpad keys
	if (keypad(input_win, TRUE) == ERR) {
		status_write("[error] could not enable extended keys");
	}

	return 1;
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
