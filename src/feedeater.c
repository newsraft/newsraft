#include <ncurses.h>
#include "feedeater.h"
int main (int argc, char **argv) {
	initscr(); // initialize curses mode
	noecho(); // do not print characters typed be the user
	//raw(); noecho(); keypad(stdscr, TRUE); getmaxyx(stdscr, row, col);
	load_feeds();
	show_feeds();
	menu_feeds();
	close_feeds();
	endwin(); // end curses mode
	return 0;
}
