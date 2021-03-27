#include "feedeater.h"

int main (int argc, char **argv) {

	int error = 0;

	if (load_feed_list() == 0) { error = 1; goto undo1; }

	// initialize curses mode
	if (initscr() == NULL) { error = 2; goto undo1; }

	// do not print characters typed be the user
	if (noecho() == ERR) { error = 3; goto undo2; }

	// disable line buffering and erase/kill character-processing
	if (cbreak() == ERR) { error = 4; goto undo2; } 

	if (status_create() == 0) { error = 5; goto undo3; }

	if (input_create() == 0) { error = 6; goto undo4; }

	feeds_menu();

undo4:
	input_delete();
undo3:
	status_delete();
undo2:
	// end curses mode
	endwin();
undo1:
	free_feed_list();

	return error;
}
