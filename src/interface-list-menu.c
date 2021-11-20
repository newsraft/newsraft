#include <stdlib.h>
#include "feedeater.h"

static WINDOW **windows = NULL;
static int windows_count;
int list_menu_height;
int list_menu_width;

void
free_list_menu(void)
{
	for (int i = 0; i < windows_count; ++i) {
		delwin(windows[i]);
	}
	free(windows);
}

static int
get_terminal_size(void)
{
	list_menu_height = getmaxy(stdscr) - 1;
	if ((list_menu_height + 1) == ERR) {
		debug_write(DBG_FAIL, "Failed to get height of the terminal!\n");
		return 1; // failure
	}
	if (list_menu_height < 1) {
		debug_write(DBG_FAIL, "Terminal is too short!\n");
		return 1; // failure
	}

	list_menu_width = getmaxx(stdscr);
	if (list_menu_width == ERR) {
		debug_write(DBG_FAIL, "Failed to get width of the terminal!\n");
		return 1; // failure
	}
	if (list_menu_width < 4) {
		debug_write(DBG_FAIL, "Terminal is too narrow!\n");
		return 1; // failure
	}

	return 0; // success
}

int
create_list_menu(void)
{
	if (get_terminal_size() != 0) {
		fprintf(stderr, "failed to get terminal size\n");
		return 1; // failure
	}

	windows_count = list_menu_height;
	windows = malloc(sizeof(WINDOW *) * windows_count);
	if (windows == NULL) {
		fprintf(stderr, "not enough memory for creating list menu\n");
		return 1; // failure
	}
	for (int i = 0; i < windows_count; ++i) {
		windows[i] = newwin(1, list_menu_width, i, 0);
		if (windows[i] == NULL) {
			free_list_menu();
			return 1; // failure
		}
	}
	return 0; // success
}

void
resize_list_menu(void)
{
	get_terminal_size();

	int new_windows_count = list_menu_height;
	if (new_windows_count > windows_count) {
		windows = realloc(windows, sizeof(WINDOW *) * new_windows_count);
		for (int i = windows_count - 1; i < new_windows_count; ++i) {
			windows[i] = newwin(1, list_menu_width, i, 0);
		}
	}
	// if (new_windows_count < windows_count) {
	//     You might think that these windows that are not needed anymore should be deleted,
	//     but this function is called on screen resize, on which ncurses automatically
	//     deletes everything that entirely gone out of bounds. Hence do nothing in this case...
	// }
	windows_count = new_windows_count;
	refresh();
}

WINDOW *
get_list_entry_by_index(size_t index)
{
	if (index < (size_t)windows_count) {
		return windows[index]; // success
	}
	return NULL; // failure
}
