#include "feedeater.h"

WINDOW *input_win = NULL;

int
input_create(void)
{
	input_win = newwin(1, 1, LINES, COLS);
	if (input_win == NULL) {
		return 1;
	}

	if (noecho() == ERR) {
		fprintf(stderr, "noecho() failed => can't disable echoing of characters typed be the user!\n");
	}
	if (cbreak() == ERR) {
		fprintf(stderr, "cbreak() failed => can't disable line buffering and erase/kill character-processing!\n");
	} 
	if (keypad(input_win, TRUE) == ERR) {
		fprintf(stderr, "keypad(input_win, TRUE) failed => can't enable extended keys!\n");
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
