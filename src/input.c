#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "newsraft.h"

struct input_binding {
	char *key;
	input_cmd_id cmd;
	struct wstring *cmdcmd;
};

static struct input_binding *binds = NULL;
static size_t binds_count = 0;

static volatile bool system_wants_us_to_terminate = false;

int
get_input_command(uint32_t *count, const struct wstring **macro_ptr)
{
	if (system_wants_us_to_terminate == true) {
		INFO("Received a signal which is asking us to terminate the program.");
		return INPUT_QUIT_HARD;
	}

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
			*macro_ptr = binds[i].cmdcmd;
			return binds[i].cmd;
		}
	}

	return INPUTS_COUNT; // No command matched with this key.
}

void
tell_program_to_terminate_safely_and_quickly(int dummy)
{
	(void)dummy;
	system_wants_us_to_terminate = true;
}

static inline int64_t
find_bind_by_its_key_name_or_create_it(const char *key_name, size_t key_name_len)
{
	int64_t i;
	for (i = 0; (size_t)i < binds_count; ++i) {
		if (strcmp(key_name, binds[i].key) == 0) {
			return i;
		}
	}
	struct input_binding *tmp = realloc(binds, sizeof(struct input_binding) * (binds_count + 1));
	if (tmp == NULL) {
		return -1;
	}
	binds_count += 1;
	binds = tmp;
	binds[i].key = malloc(sizeof(char) * (key_name_len + 1));
	if (binds[i].key == NULL) {
		return -1;
	}
	memcpy(binds[i].key, key_name, sizeof(char) * key_name_len);
	binds[i].key[key_name_len] = '\0';
	binds[i].cmdcmd = NULL; // Set to NULL to avoid freeing garbage below.
	return i;
}

bool
assign_action_to_key(const char *bind_key, size_t bind_key_len, input_cmd_id bind_cmd)
{
	INFO("Binding %u action to \"%s\" key.", bind_cmd, bind_key);
	int64_t index = find_bind_by_its_key_name_or_create_it(bind_key, bind_key_len);
	if (index == -1) {
		fputs("Not enough memory for assigning an action to a new key!\n", stderr);
		return false;
	}
	free_wstring(binds[index].cmdcmd);
	binds[index].cmd = bind_cmd;
	binds[index].cmdcmd = NULL;
	return true;
}

bool
create_macro(const char *bind_key, size_t bind_key_len, const char *cmd, size_t cmd_len)
{
	INFO("Binding \"%s\" command to \"%s\" key.", cmd, bind_key);
	int64_t index = find_bind_by_its_key_name_or_create_it(bind_key, bind_key_len);
	if (index == -1) {
		goto error;
	}
	free_wstring(binds[index].cmdcmd);
	binds[index].cmd = INPUT_SYSTEM_COMMAND;
	binds[index].cmdcmd = NULL; // Set to NULL because stuff below can fail.
	struct string *cmd_str = crtas(cmd, cmd_len);
	if (cmd_str == NULL) {
		goto error;
	}
	binds[index].cmdcmd = convert_string_to_wstring(cmd_str);
	free_string(cmd_str);
	if (binds[index].cmdcmd == NULL) {
		goto error;
	}
	return true;
error:
	fputs("Not enough memory for assigning a command to a new key!\n", stderr);
	return false;
}

void
delete_action_from_key(const char *bind_key)
{
	for (size_t i = 0; i < binds_count; ++i) {
		if (strcmp(bind_key, binds[i].key) == 0) {
			binds_count -= 1;
			free(binds[i].key);
			free_wstring(binds[i].cmdcmd);
			for (size_t j = i; j < binds_count; ++j) {
				binds[j].key = binds[j + 1].key;
				binds[j].cmd = binds[j + 1].cmd;
				binds[j].cmdcmd = binds[j + 1].cmdcmd;
			}
			return;
		}
	}
}

void
free_binds(void)
{
	INFO("Freeing key binds.");
	for (size_t i = 0; i < binds_count; ++i) {
		free(binds[i].key);
		free_wstring(binds[i].cmdcmd);
	}
	free(binds);
}

bool
assign_default_binds(void)
{
	if (assign_action_to_key("j",              1, INPUT_SELECT_NEXT)         == false) { goto error; }
	if (assign_action_to_key("KEY_DOWN",       8, INPUT_SELECT_NEXT)         == false) { goto error; }
	if (assign_action_to_key("n",              1, INPUT_SELECT_NEXT_UNREAD)  == false) { goto error; }
	if (assign_action_to_key("KEY_NPAGE",      9, INPUT_SELECT_NEXT_PAGE)    == false) { goto error; }
	if (assign_action_to_key("k",              1, INPUT_SELECT_PREV)         == false) { goto error; }
	if (assign_action_to_key("KEY_UP",         6, INPUT_SELECT_PREV)         == false) { goto error; }
	if (assign_action_to_key("p",              1, INPUT_SELECT_PREV_UNREAD)  == false) { goto error; }
	if (assign_action_to_key("KEY_PPAGE",      9, INPUT_SELECT_PREV_PAGE)    == false) { goto error; }
	if (assign_action_to_key("g",              1, INPUT_SELECT_FIRST)        == false) { goto error; }
	if (assign_action_to_key("KEY_HOME",       8, INPUT_SELECT_FIRST)        == false) { goto error; }
	if (assign_action_to_key("G",              1, INPUT_SELECT_LAST)         == false) { goto error; }
	if (assign_action_to_key("KEY_END",        7, INPUT_SELECT_LAST)         == false) { goto error; }
	if (assign_action_to_key("l",              1, INPUT_ENTER)               == false) { goto error; }
	if (assign_action_to_key("^J",             2, INPUT_ENTER)               == false) { goto error; }
	if (assign_action_to_key("KEY_RIGHT",      9, INPUT_ENTER)               == false) { goto error; }
	if (assign_action_to_key("KEY_ENTER",      9, INPUT_ENTER)               == false) { goto error; }
	if (assign_action_to_key("r",              1, INPUT_RELOAD)              == false) { goto error; }
	if (assign_action_to_key("^R",             2, INPUT_RELOAD_ALL)          == false) { goto error; }
	if (assign_action_to_key("d",              1, INPUT_MARK_READ)           == false) { goto error; }
	if (assign_action_to_key("^D",             2, INPUT_MARK_READ_ALL)       == false) { goto error; }
	if (assign_action_to_key("D",              1, INPUT_MARK_UNREAD)         == false) { goto error; }
	if (assign_action_to_key("i",              1, INPUT_MARK_IMPORTANT)      == false) { goto error; }
	if (assign_action_to_key("I",              1, INPUT_MARK_UNIMPORTANT)    == false) { goto error; }
	if (assign_action_to_key("e",              1, INPUT_EXPLORE_MENU)        == false) { goto error; }
	if (assign_action_to_key("v",              1, INPUT_STATUS_HISTORY_MENU) == false) { goto error; }
	if (assign_action_to_key("o",              1, INPUT_OPEN_IN_BROWSER)     == false) { goto error; }
	if (assign_action_to_key("y",              1, INPUT_COPY_TO_CLIPBOARD)   == false) { goto error; }
	if (assign_action_to_key("c",              1, INPUT_COPY_TO_CLIPBOARD)   == false) { goto error; }
	if (assign_action_to_key("h",              1, INPUT_QUIT_SOFT)           == false) { goto error; }
	if (assign_action_to_key("q",              1, INPUT_QUIT_SOFT)           == false) { goto error; }
	if (assign_action_to_key("KEY_LEFT",       8, INPUT_QUIT_SOFT)           == false) { goto error; }
	if (assign_action_to_key("KEY_BACKSPACE", 13, INPUT_QUIT_SOFT)           == false) { goto error; }
	if (assign_action_to_key("Q",              1, INPUT_QUIT_HARD)           == false) { goto error; }
	return true;
error:
	fputs("Failed to assign default bindings!\n", stderr);
	free_binds();
	return false;
}
