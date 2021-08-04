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
	if (LINES > windows_count) {
		windows = realloc(windows, sizeof(WINDOW *) * (LINES - 1));
		for (int i = windows_count - 1; i < (LINES - 1); ++i) {
			windows[i] = newwin(1, COLS, i, 0);
		}
	} else if (LINES < windows_count) {
		/* you might think that these windows that are not needed should be deleted.
		 * but this function is called on screen resize, on which ncurses automatically
		 * deletes all the windows that have gone out of bounds.
		 * hence do nothing in this case... */
	}
	windows_count = LINES - 1;
}

WINDOW *
get_list_entry_by_index(size_t index)
{
	return index < (size_t)windows_count ? windows[index] : NULL;
}
