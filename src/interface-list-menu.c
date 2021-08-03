#include "feedeater.h"

static WINDOW **windows = NULL;
static size_t windows_count;

void
free_list_menu(void)
{
	for (size_t i = 0; i < windows_count; ++i) {
		delwin(windows[i]);
	}
	free(windows);
}

int
create_list_menu(void)
{
	if (LINES < 1) {
		return 1;
	}
	for (windows_count = 0; windows_count < (size_t)(LINES - 1); ++windows_count) {
		windows = realloc(windows, sizeof(WINDOW *) * (windows_count + 1));
		if (windows == NULL) {
			return 1;
		}
		windows[windows_count] = newwin(1, COLS, windows_count, 0);
		if (windows[windows_count] == NULL) {
			free_list_menu();
			return 1;
		}
	}
	return 0;
}

WINDOW *
get_list_entry_by_index(size_t i)
{
	return i < windows_count ? windows[i] : NULL;
}
