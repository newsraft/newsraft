#include "feedeater.h"
#include "db/db.h"
int main (int argc, char **argv) {
	int error = 0;
	if (db_init() != 0) { error = 1; goto undo1; }
	if (load_feed_list() != 0) { error = 2; goto undo2; }

	// initialize curses mode
	if (initscr() == NULL) { error = 3; goto undo2; }

	// do not print characters typed be the user
	if (noecho() == ERR) { error = 4; goto undo3; }

	// disable line buffering and erase/kill character-processing
	if (cbreak() == ERR) { error = 5; goto undo3; } 

	if (status_create() == 0) { error = 6; goto undo3; }
	if (input_create() == 0) { error = 7; goto undo4; }
	feeds_menu();
	input_delete();
undo4:
	status_delete();
undo3:
	// end curses mode
	endwin();
undo2:
	db_stop();
undo1:
	return error;
}
