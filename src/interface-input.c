#include <stdlib.h>
#include "feedeater.h"

// Linked list
struct input_binding {
	int key;
	enum input_cmd cmd;
	struct input_binding *next_bind;
};

static struct input_binding *head_bind = NULL;

// Array of function pointers to input handlers.
static void (*input_handlers[INPUTS_COUNT])(void);

void
reset_input_handlers(void)
{
	for (int i = 0; i < INPUTS_COUNT; ++i) {
		input_handlers[i] = NULL;
	}
}

static inline int
get_input_command(void)
{
	int c = getch();

	struct input_binding *bind = head_bind;
	while (bind != NULL) {
		if (c == bind->key) {
			return bind->cmd;
		}
		bind = bind->next_bind;
	}

	if (c == KEY_RESIZE) {
		// If getch() returns KEY_RESIZE, then user's terminal got resized.
		// We have to redraw everything that is in the view right now, and
		// also make inactive interfaces know that they must do their resize
		// callback on another activation.

		if (obtain_terminal_size() == false) {
			// Some really crazy resize happend. This state is really
			// dangerous, better just break all loops and exit the program.
			return INPUT_QUIT_HARD;
		}

		// Rearrange list menu windows.
		adjust_list_menu();
		resize_sets_global_action();
		resize_items_global_action();

		reallocate_format_buffer();

		/* recreate a status window */
		status_delete();
		status_create();
		status_clean();

		return INPUT_RESIZE;
	}

	return INPUTS_COUNT; // no command matched with this key
}

int
handle_input(void)
{
	int cmd;
	while (1) {
		cmd = get_input_command();
		if (cmd == INPUT_ENTER || cmd == INPUT_QUIT_SOFT || cmd == INPUT_QUIT_HARD) {
			return cmd;
		} else if (cmd == INPUTS_COUNT || input_handlers[cmd] == NULL) {
			continue;
		}
		input_handlers[cmd]();
	}
}

void
set_input_handler(enum input_cmd cmd, void (*func)(void))
{
	if (cmd < 0 || cmd >= INPUTS_COUNT) {
		return;
	}
	input_handlers[cmd] = func;
}

int
assign_command_to_key(int bind_key, enum input_cmd bind_cmd)
{
	INFO("Trying to bind a key with the code %d.", bind_key);

	if (bind_key == KEY_RESIZE) {
		WARN("Key with the code KEY_RESIZE must not be bound!");
		return 1; // failure
	}

	/* Check if bind_key key is bound already */
	struct input_binding *bind = head_bind;
	while (bind != NULL) {
		if (bind_key == bind->key) {
			INFO("Key with the code %d is already bound. Reassigning a command.", bind_key);
			bind->cmd = bind_cmd;
			return 0; // success
		}
		bind = bind->next_bind;
	}

	bind = malloc(sizeof(struct input_binding));
	if (bind == NULL) {
		fprintf(stderr, "Not enough memory for assigning a command to a new key!\n");
		return 1;
	}

	bind->key = bind_key;
	bind->cmd = bind_cmd;
	bind->next_bind = head_bind;
	head_bind = bind;

	INFO("Key with the code %d is successfully bound!", bind_key);

	return 0; // success
}

void
free_binds(void)
{
	INFO("Freeing key binds.");
	struct input_binding *bind = head_bind;
	struct input_binding *tmp;
	while (bind != NULL) {
		tmp = bind;
		bind = bind->next_bind;
		free(tmp);
	}
}
