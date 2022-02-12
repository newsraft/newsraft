#include <stdlib.h>
#include <stdio.h>
#include "feedeater.h"

struct input_binding {
	int key;
	enum input_cmd cmd;
};

static struct input_binding *binds = NULL;
static size_t binds_count = 0;

int
get_input_command(void)
{
	int c = getch();

	for (size_t i = 0; i < binds_count; ++i) {
		if (c == binds[i].key) {
			return binds[i].cmd;
		}
	}

	if (c == KEY_RESIZE) {
		// If getch() returns KEY_RESIZE, then user's terminal got resized.

		if (obtain_terminal_size() == false) {
			// Some really crazy resize happend. It is either a glitch or
			// user deliberately trying to break something. This state is really
			// dangerous anyways, better just break all loops and exit the program.
			fprintf(stderr, "Don't flex around with me, okay?\n");
			return INPUT_QUIT_HARD;
		}

		adjust_list_menu();

		if (adjust_list_menu_format_buffer() == false) {
			return INPUT_QUIT_HARD;
		}

		status_resize();

		return INPUT_RESIZE;
	}

	return INPUTS_COUNT; // no command matched with this key
}

bool
assign_action_to_key(int bind_key, enum input_cmd bind_cmd)
{
	INFO("Binding %u action to %d key.", bind_cmd, bind_key);

	if (bind_key == KEY_RESIZE) {
		WARN("Key with the code KEY_RESIZE must not be bound!");
		return false;
	}

	// Check if bind_key is bound already.
	for (size_t i = 0; i < binds_count; ++i) {
		if (bind_key == binds[i].key) {
			INFO("Key with the code %d is already bound. Reassigning a command.", bind_key);
			binds[i].cmd = bind_cmd;
			return true;
		}
	}

	size_t bind_index = binds_count++;
	struct input_binding *temp = realloc(binds, sizeof(struct input_binding) * binds_count);
	if (temp == NULL) {
		fprintf(stderr, "Not enough memory for assigning a command to a new key!\n");
		return false;
	}
	binds = temp;

	binds[bind_index].key = bind_key;
	binds[bind_index].cmd = bind_cmd;

	return true;
}

void
free_binds(void)
{
	INFO("Freeing key binds.");
	free(binds);
}
