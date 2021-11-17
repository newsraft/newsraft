#include <stdarg.h>
#include "feedeater.h"

static WINDOW *status_win = NULL; 

int
status_create(void)
{
	status_win = newwin(1, list_menu_width, list_menu_height, 0); // create status window
	if (status_win == NULL) {
		fprintf(stderr, "could not create status line\n");
		return 1; // failure
	}
	if (cbreak() == ERR) {
		fprintf(stderr, "Can not disable line buffering and erase/kill character-processing\n");
		return 1; // failure
	}
	if (curs_set(0) == ERR) { // try to hide cursor
		debug_write(DBG_FAIL, "Can not hide cursor\n");
	}
	if (noecho() == ERR) {
		debug_write(DBG_FAIL, "Can not disable echoing of characters typed by the user\n");
	}
	if (keypad(stdscr, TRUE) == ERR) { // used to enable arrow keys, function keys...
		debug_write(DBG_FAIL, "Can not enable extended keys\n");
	}
	return 0; // success
}

void
status_write(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	werase(status_win);
	wmove(status_win, 0, 0);
	vw_printw(status_win, format, args);
	wrefresh(status_win);
	va_end(args);
}

void
status_clean(void)
{
	werase(status_win);
	wrefresh(status_win);
}

void
status_delete(void)
{
	delwin(status_win);
}
