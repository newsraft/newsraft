#include "feedeater.h"

static WINDOW **windows = NULL;
static int windows_count;

void
free_list_menu(void)
{
	for (int i = 0; i < windows_count; ++i) {
		delwin(windows[i]);
	}
	free(windows);
}

int
create_list_menu(void)
{
	if (LINES < 2) {
		return 1;
	}
	windows_count = LINES - 1;
	windows = realloc(windows, sizeof(WINDOW *) * windows_count);
	if (windows == NULL) {
		return 1;
	}
	for (int i = 0; i < windows_count; ++i) {
		windows[i] = newwin(1, COLS, i, 0);
		if (windows[i] == NULL) {
			free_list_menu();
			return 1;
		}
	}
	return 0;
}

void
resize_list_menu(void)
{
	int new_windows_count = LINES - 1;
	if (new_windows_count > windows_count) {
		windows = realloc(windows, sizeof(WINDOW *) * new_windows_count);
		for (int i = windows_count - 1; i < new_windows_count; ++i) {
			windows[i] = newwin(1, COLS, i, 0);
		}
	} else if (new_windows_count < windows_count) {
		/* You might think that these windows that are not needed anymore should be deleted,
		 * but this function is called on screen resize, on which ncurses automatically
		 * deletes everything that gone out of bounds. Hence do nothing in this case... */
	}
	windows_count = new_windows_count;
}

WINDOW *
get_list_entry_by_index(size_t index)
{
	return index < (size_t)windows_count ? windows[index] : NULL;
}
