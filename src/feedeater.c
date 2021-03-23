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

	show_feeds();
	menu_feeds();

undo3:
	status_delete();
undo2:
	endwin(); // end curses mode
undo1:
	free_feed_list();
	return error;
}
