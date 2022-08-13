#include <stdio.h>
#include <stdlib.h>
#include "newsraft.h"

static bool
obtain_terminal_size(void)
{
	int terminal_width = getmaxx(stdscr);
	if (terminal_width == ERR) {
		FAIL("Failed to get width of the terminal!");
		return false;
	}
	if (terminal_width < 5) {
		FAIL("Terminal is too narrow!");
		return false;
	}
	int terminal_height = getmaxy(stdscr);
	if (terminal_height == ERR) {
		FAIL("Failed to get height of the terminal!");
		return false;
	}
	if (terminal_height < 5) {
		FAIL("Terminal is too short!");
		return false;
	}

	INFO("Obtained terminal size: %d width, %d height.", terminal_width, terminal_height);

	list_menu_width = terminal_width;
	list_menu_height = terminal_height - 1; // Subtract 1 because we have status window.

	return true;
}

bool
curses_init(void)
{
	if (initscr() == NULL) {
		fputs("Initialization of curses data structures failed!\n", stderr);
		return false;
	}
	if (cbreak() == ERR) {
		fputs("Can not disable line buffering and erase/kill character-processing!\n", stderr);
		return false;
	}
	if (obtain_terminal_size() == false) {
		fputs("Invalid terminal size obtained!\n", stderr);
		return false;
	}
	if (curs_set(0) == ERR) {
		WARN("Can't hide cursor!");
	}
	if (noecho() == ERR) {
		WARN("Can't disable echoing of characters typed by the user!");
	}
	if (getenv("NO_COLOR") != NULL) {
		INFO("NO_COLOR environment variable is set, canceling colors initialization.");
	} else if (start_color() == ERR) {
		WARN("Initialization of curses color structures failed!");
	} else if (create_color_pairs() == false) {
		WARN("Can't create color pairs!");
	}
	return true;
}

bool
resize_counter_action(void)
{
	if (obtain_terminal_size() == false) {
		// Some really crazy resize happend. It is either a glitch or user
		// deliberately trying to break something. This state is really
		// dangerous anyways.
		fputs("Don't flex around with me, okay?\n", stderr);
		return false;
	}
	if (adjust_list_menu() == false) {
		return false;
	}
	if (status_resize() == false) {
		return false;
	}
	if (counter_resize() == false) {
		return false;
	}
	redraw_list_menu();
	return true;
}
