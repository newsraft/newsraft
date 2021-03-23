#include <ncurses.h>
#include <stdarg.h>
#include "feedeater.h"

static WINDOW *status_win = NULL; 

int status_create(void)
{
	status_win = newwin(1, COLS, LINES - 1, 0); // create status window
	if (status_win == NULL) {
		fprintf(stderr, "could not create status line\n");
		return 0;
	}
	return 1;
}

void
status_write(char *format, ...)
{
	va_list args;
	va_start(args, format);
	wclear(status_win);
	wmove(status_win, 0, 0);
	vw_printw(status_win, format, args);
	wrefresh(status_win);
	va_end(args);
}

int status_delete(void)
{
	return delwin(status_win);
}
