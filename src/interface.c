#include <stdlib.h>
#include "newsraft.h"

bool search_mode_is_enabled = false;
struct string *search_mode_text_input = NULL;

pthread_mutex_t interface_lock = PTHREAD_MUTEX_INITIALIZER;

static bool
obtain_list_menu_size(size_t *width, size_t *height)
{
	int terminal_width = getmaxx(stdscr);
	if (terminal_width == ERR) {
		FAIL("Failed to get width of the terminal!");
		return false;
	}
	// It is necessary that a counter field of 9 characters width and at least
	// some nonzero width of the status field fit in the screen.
	// This is really critical! You will get integer overflow if terminal_width
	// is less than 10. You have been warned.
	if (terminal_width < 10) {
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

	*width = terminal_width;
	*height = terminal_height - 1; // Subtract 1 because we have status window.

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
	if (obtain_list_menu_size(&list_menu_width, &list_menu_height) == false) {
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
	} else if (has_colors() == FALSE) {
		WARN("Terminal emulator doesn't support colors!");
	} else if (use_default_colors() == ERR) {
		WARN("Can't obtain default terminal colors!");
	} else if (start_color() == ERR) {
		WARN("Initialization of curses color structures failed!");
	} else if (create_color_pairs() == false) {
		WARN("Can't create color pairs!");
	}
	INFO("The value of KEY_RESIZE code is %d.", KEY_RESIZE);
	return true;
}

input_cmd_id
resize_handler(void)
{
	pthread_mutex_lock(&interface_lock);

	// We need to call clear and refresh before all resize actions because
	// the junk text of previous size may remain in inactive areas.
	clear();
	refresh();

	if (obtain_list_menu_size(&list_menu_width, &list_menu_height) == false) {
		// Some really crazy resize happend. It is either a glitch or user
		// deliberately trying to break something. This state is unusable anyways.
		fputs("Don't flex around with me, okay?\n", stderr);
		goto error;
	}
	if (adjust_list_menu() == false) {
		goto error;
	}
	if (status_recreate_unprotected() == false) {
		goto error;
	}
	if (counter_recreate_unprotected() == false) {
		goto error;
	}
	if (get_current_menu_type() == PAGER_MENU) {
		refresh_pager_menu();
	}
	redraw_list_menu_unprotected();
	pthread_mutex_unlock(&interface_lock);
	return INPUT_ERROR;
error:
	tell_program_to_terminate_safely_and_quickly(0);
	pthread_mutex_unlock(&interface_lock);
	return INPUT_QUIT_HARD;
}

bool
call_resize_handler_if_current_list_menu_size_is_different_from_actual(void)
{
	size_t width = 0, height = 0;
	obtain_list_menu_size(&width, &height);
	if (width != list_menu_width || height != list_menu_height) {
		resize_handler();
		return true;
	}
	return false;
}

struct menu_state *
setup_menu(struct menu_state *(*menu)(struct menu_state *), struct feed_entry **feeds, size_t feeds_count, uint32_t flags)
{
	static struct menu_state *head = NULL;
	// Get up-to-date information on unread items count
	if (head != NULL && head->feeds != NULL && head->feeds_count > 0) {
		for (size_t i = 0; i < head->feeds_count; ++i) {
			int64_t unread_count = get_unread_items_count_of_the_feed(head->feeds[i]->link);
			if (unread_count >= 0) {
				head->feeds[i]->unread_count = unread_count;
			}
		}
	}
	if (menu != NULL) {
		struct menu_state *new = malloc(sizeof(struct menu_state));
		new->run         = menu;
		new->feeds       = feeds;
		new->feeds_count = feeds_count;
		new->flags       = flags;
		new->caller      = head != NULL ? head->run : NULL;
		if ((flags & MENU_SWALLOW) && head != NULL) {
			new->next = head->next;
			free(head);
		} else {
			new->next = head;
		}
		head = new;
	} else if (head != NULL) {
		struct menu_state *tmp = head;
		head = head->next;
		free(tmp);
	}
	return head;
}
