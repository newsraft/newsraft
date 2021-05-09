#include "feedeater.h"
#include <locale.h>
int main (int argc, char **argv) {
	setlocale(LC_ALL, "");
	int error = 0;
	if (set_config_dir_path() != 0) { error = 1; goto undo1; }
	if (set_data_dir_path() != 0)   { error = 2; goto undo2; }
	if (db_init() != 0)             { error = 3; goto undo3; }
	if (load_feed_list() != 0)      { error = 4; goto undo4; }
	if (initscr() == NULL)          { error = 5; goto undo5; } // init curses mode
	if (status_create() != 0)       { error = 6; goto undo6; }
	if (input_create() != 0)        { error = 7; goto undo7; }
	run_feeds_menu();
	input_delete();
undo7:
	status_delete();
undo6:
	endwin(); // stop curses mode
undo5:
	free_feed_list();
undo4:
	db_stop();
undo3:
	free_data_dir_path();
undo2:
	free_config_dir_path();
undo1:
	return error;
}
