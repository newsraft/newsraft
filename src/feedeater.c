#include <ncurses.h>
#include "feedeater.h"
int main (int argc, char **argv) {
	initscr(); // initialize curses mode
	noecho(); // do not print characters typed be the user
	cbreak(); // disable line buffering and erase/kill character-processing
	//raw(); keypad(stdscr, TRUE);
	load_feeds();
	show_feeds();
	menu_feeds();
	close_feeds();
	endwin(); // end curses mode
	return 0;
}
