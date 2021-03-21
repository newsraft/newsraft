#include <ncurses.h>
#include "feedeater.h"

WINDOW *status_win = NULL; 

int main (int argc, char **argv) {
	if (load_feeds() == 0) { close_feeds(); return 1; }
	initscr(); // initialize curses mode
	noecho(); // do not print characters typed be the user
	cbreak(); // disable line buffering and erase/kill character-processing
	status_win = newwin(1, COLS, LINES - 1, 0); // create status window
	show_feeds();
	menu_feeds();
	close_feeds();
	delwin(status_win); // delete status window
	endwin(); // end curses mode
	return 0;
}
