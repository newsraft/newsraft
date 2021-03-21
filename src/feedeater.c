#include <ncurses.h>
#include "feedeater.h"
WINDOW *status_win = NULL; 
int main (int argc, char **argv) {
	initscr(); // initialize curses mode
	noecho(); // do not print characters typed be the user
	cbreak(); // disable line buffering and erase/kill character-processing
	status_win = newwin(1, COLS, LINES - 1, 0);
	load_feeds();
	show_feeds();
	menu_feeds();
	close_feeds();
	delwin(status_win);
	endwin(); // end curses mode
	return 0;
}
