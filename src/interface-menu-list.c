#include <stdlib.h>
#include "feedeater.h"

static WINDOW **windows = NULL;
static size_t windows_count = 0;
size_t list_menu_height;
size_t list_menu_width;

void
free_list_menu(void)
{
	for (size_t i = 0; i < windows_count; ++i) {
		delwin(windows[i]);
	}
	free(windows);
}

bool
adjust_list_menu(void)
{
	if (list_menu_height > windows_count) {
		WINDOW **temp = realloc(windows, sizeof(WINDOW *) * list_menu_height);
		if (temp == NULL) {
			FAIL("Not enough memory for reallocating list menu windows!");
			return false;
		}
		windows = temp;
		for (size_t i = windows_count; i < list_menu_height; ++i) {
			windows[i] = newwin(1, list_menu_width, i, 0);
			if (windows[i] == NULL) {
				FAIL("Not enough memory for window to adjust list menu!");
				windows_count = i;
				free_list_menu();

				// Set these to 0 because free_list_menu() will
				// be called again in the end of main(). That way we will
				// avoid double free. Yeah, yeah, looks "very" ineffective,
				// but what are the odds of shortage of memory on list menu
				// reallocation?
				windows_count = 0;
				windows = NULL;

				return false;
			}
		}
	}
	// if (list_menu_height < windows_count) {
	//     You might think that these windows that are not needed anymore should be deleted,
	//     but this function is called on screen resize, on which ncurses automatically
	//     deletes everything that entirely gone out of bounds. Hence do nothing in this case...
	// }
	windows_count = list_menu_height;
	refresh();
	return true;
}

WINDOW *
get_list_entry_by_index(size_t index)
{
	if (index < windows_count) {
		return windows[index];
	}
	return NULL;
}
