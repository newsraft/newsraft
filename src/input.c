#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "newsraft.h"

struct input_binding {
	char *key;
	input_cmd_id cmd;
};

static struct input_binding *binds = NULL;
static size_t binds_count = 0;

int
get_input_command(uint32_t *count, const struct wstring **macro_ptr)
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
			return INPUT_QUIT_HARD; // Achtung!
		}
	}

	*count = counter_extract_count();
	counter_clean();

	const char *key = keyname(c);
	for (size_t i = 0; i < binds_count; ++i) {
		if (strcmp(key, binds[i].key) == 0) {
			return binds[i].cmd;
		}
	}

	*macro_ptr = find_macro(key);

	return INPUTS_COUNT; // No command matched with this key.
}

bool
assign_action_to_key(const char *bind_key, size_t bind_key_len, input_cmd_id bind_cmd)
{
	INFO("Binding %u action to \"%s\" key.", bind_cmd, bind_key);

	// Check if bind_key is bound already.
	for (size_t i = 0; i < binds_count; ++i) {
		if (strcmp(bind_key, binds[i].key) == 0) {
			INFO("Key with name \"%s\" is already bound. Reassigning the command.", bind_key);
			binds[i].cmd = bind_cmd;
			return true;
		}
	}

	size_t bind_index = binds_count++;
	struct input_binding *temp = realloc(binds, sizeof(struct input_binding) * binds_count);
	if (temp == NULL) {
		goto error;
	}

	binds = temp;
	binds[bind_index].key = malloc(sizeof(char) * (bind_key_len + 1));
	if (binds[bind_index].key == NULL) {
		goto error;
	}
	strncpy(binds[bind_index].key, bind_key, bind_key_len);
	binds[bind_index].key[bind_key_len] = '\0';
	binds[bind_index].cmd = bind_cmd;

	return true;
error:
	fputs("Not enough memory for assigning a command to a new key!\n", stderr);
	return false;
}

void
free_binds(void)
{
	INFO("Freeing key binds.");
	for (size_t i = 0; i < binds_count; ++i) {
		free(binds[i].key);
	}
	free(binds);
}
