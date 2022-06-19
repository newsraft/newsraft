#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "newsraft.h"

struct input_binding {
	int key;
	enum input_cmd cmd;
};

static struct input_binding *binds = NULL;
static size_t binds_count = 0;

int
get_input_command(uint32_t *count)
{
	int c = read_key_from_status();

	while (isdigit(c) != 0) {
		counter_send_character(c);
		c = read_key_from_status();
	}

	if (c == KEY_RESIZE) {
		INFO("Apparently, resize action code is %d.", c);
		if (resize_counter_action() == true) {
			return INPUT_RESIZE;
		} else {
			return INPUT_QUIT_HARD;
		}
	}

	*count = counter_extract_count();
	counter_clean();

	for (size_t i = 0; i < binds_count; ++i) {
		if (c == binds[i].key) {
			return binds[i].cmd;
		}
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
		fputs("Not enough memory for assigning a command to a new key!\n", stderr);
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
