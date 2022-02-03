#include "feedeater.h"

static WINDOW *status_win = NULL; 
static struct string *status_buf = NULL;

bool
status_create(void)
{
	status_win = newwin(1, list_menu_width, list_menu_height, 0);
	if (status_win == NULL) {
		fprintf(stderr, "Failed to create status line window!\n");
		return false;
	}
	status_buf = crtes();
	if (status_buf == NULL) {
		fprintf(stderr, "Failed to create status line buffer!\n");
		delwin(status_win);
		return false;
	}
	return true;
}

void
status_update(void)
{
	werase(status_win);
	mvwaddnstr(status_win, 0, 0, status_buf->ptr, list_menu_width);
	wrefresh(status_win);
}

void
status_write(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	if (string_vprintf(status_buf, format, args) == false) {
		return;
	}
	INFO("Wrote to status: %s", status_buf->ptr);
	status_update();
	va_end(args);
}

void
status_clean(void)
{
	werase(status_win);
	wrefresh(status_win);
	empty_string(status_buf);
}

void
status_resize(void)
{
	if (status_win != NULL) {
		delwin(status_win);
	}
	status_win = newwin(1, list_menu_width, list_menu_height, 0);
	if (status_win == NULL) {
		return;
	}
	status_update();
}

void
status_delete(void)
{
	delwin(status_win);
	free_string(status_buf);
}
