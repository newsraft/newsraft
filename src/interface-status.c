#include <stdarg.h>
#include "feedeater.h"

static WINDOW *status_win = NULL; 

bool
status_create(void)
{
	status_win = newwin(1, list_menu_width, list_menu_height, 0);
	if (status_win == NULL) {
		fprintf(stderr, "Failed to create status line!\n");
		return false;
	}
	return true;
}

void
status_write(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	werase(status_win);
	wmove(status_win, 0, 0);
	if (log_stream != NULL) {
		INFO("Writing the following message to status window:");
		vfprintf(log_stream, format, args);
		fputc('\n', log_stream);
	}
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
