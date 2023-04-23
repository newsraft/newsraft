#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

struct input_binding {
	struct string *key;
	input_cmd_id cmd;
	struct wstring *cmdcmd;
};

static struct input_binding *binds = NULL;
static size_t binds_count = 0;

static volatile bool they_want_us_to_terminate = false;

input_cmd_id
get_input_command(uint32_t *count, const struct wstring **macro_ptr)
{
	if (they_want_us_to_terminate == true) {
		INFO("Received a signal which is asking us to terminate the program.");
		return INPUT_QUIT_HARD;
	}

	int c = read_counted_key_from_counter_window(count);
	if (c == KEY_RESIZE) {
		return resize_counter_action();
	}

	const char *key = keyname(c);
	for (size_t i = 0; i < binds_count; ++i) {
		if (strcmp(key, binds[i].key->ptr) == 0) {
			*macro_ptr = binds[i].cmdcmd;
			return binds[i].cmd;
		}
	}

	return INPUT_ERROR; // No command matched with this key.
}

void
tell_program_to_terminate_safely_and_quickly(int dummy)
{
	(void)dummy;
	they_want_us_to_terminate = true;
}

static inline int64_t
find_bind_by_its_key_name_or_create_it(const char *key, size_t key_len)
{
	for (size_t i = 0; i < binds_count; ++i) {
		if ((key_len == binds[i].key->len) && (memcmp(key, binds[i].key->ptr, key_len) == 0)) {
			return i;
		}
	}
	struct input_binding *tmp = realloc(binds, sizeof(struct input_binding) * (binds_count + 1));
	if (tmp != NULL) {
		binds = tmp;
		binds[binds_count].key = crtas(key, key_len);
		binds[binds_count].cmdcmd = NULL; // Set to NULL to avoid freeing garbage later.
		binds_count += 1;
		if (binds[binds_count - 1].key != NULL) {
			return binds_count - 1;
		}
	}
	fputs("Not enough memory to create another binding!\n", stderr);
	return -1;
}

bool
bind_action_to_key(const char *bind_key, size_t bind_key_len, input_cmd_id bind_cmd)
{
	int64_t index = find_bind_by_its_key_name_or_create_it(bind_key, bind_key_len);
	if (index == -1) {
		return false;
	}
	binds[index].cmd = bind_cmd;
	INFO("Binded action %u to \"%s\" key.", bind_cmd, binds[index].key->ptr);
	return true;
}

bool
create_macro(const char *bind_key, size_t bind_key_len, const char *cmd, size_t cmd_len)
{
	int64_t index = find_bind_by_its_key_name_or_create_it(bind_key, bind_key_len);
	if (index == -1) {
		return false;
	}
	free_wstring(binds[index].cmdcmd);
	binds[index].cmd = INPUT_SYSTEM_COMMAND;
	binds[index].cmdcmd = convert_array_to_wstring(cmd, cmd_len);
	if (binds[index].cmdcmd == NULL) {
		fputs("Not enough memory for assigning a command to a new key!\n", stderr);
		return false;
	}
	INFO("Binded command \"%s\" to \"%s\" key.", cmd, binds[index].key->ptr);
	return true;
}

void
free_binds(void)
{
	INFO("Freeing key binds.");
	for (size_t i = 0; i < binds_count; ++i) {
		free_string(binds[i].key);
		free_wstring(binds[i].cmdcmd);
	}
	free(binds);
}

bool
assign_default_binds(void)
{
	if (bind_action_to_key("j",         1, INPUT_SELECT_NEXT)                  == false) { goto fail; }
	if (bind_action_to_key("KEY_DOWN",  8, INPUT_SELECT_NEXT)                  == false) { goto fail; }
	if (bind_action_to_key("k",         1, INPUT_SELECT_PREV)                  == false) { goto fail; }
	if (bind_action_to_key("KEY_UP",    6, INPUT_SELECT_PREV)                  == false) { goto fail; }
	if (bind_action_to_key(" ",         1, INPUT_SELECT_NEXT_PAGE)             == false) { goto fail; }
	if (bind_action_to_key("^F",        2, INPUT_SELECT_NEXT_PAGE)             == false) { goto fail; }
	if (bind_action_to_key("KEY_NPAGE", 9, INPUT_SELECT_NEXT_PAGE)             == false) { goto fail; }
	if (bind_action_to_key("^B",        2, INPUT_SELECT_PREV_PAGE)             == false) { goto fail; }
	if (bind_action_to_key("KEY_PPAGE", 9, INPUT_SELECT_PREV_PAGE)             == false) { goto fail; }
	if (bind_action_to_key("g",         1, INPUT_SELECT_FIRST)                 == false) { goto fail; }
	if (bind_action_to_key("KEY_HOME",  8, INPUT_SELECT_FIRST)                 == false) { goto fail; }
	if (bind_action_to_key("G",         1, INPUT_SELECT_LAST)                  == false) { goto fail; }
	if (bind_action_to_key("KEY_END",   7, INPUT_SELECT_LAST)                  == false) { goto fail; }
	if (bind_action_to_key("J",         1, INPUT_JUMP_TO_NEXT)                 == false) { goto fail; }
	if (bind_action_to_key("K",         1, INPUT_JUMP_TO_PREV)                 == false) { goto fail; }
	if (bind_action_to_key("n",         1, INPUT_JUMP_TO_NEXT_UNREAD)          == false) { goto fail; }
	if (bind_action_to_key("N",         1, INPUT_JUMP_TO_PREV_UNREAD)          == false) { goto fail; }
	if (bind_action_to_key("p",         1, INPUT_JUMP_TO_NEXT_IMPORTANT)       == false) { goto fail; }
	if (bind_action_to_key("P",         1, INPUT_JUMP_TO_PREV_IMPORTANT)       == false) { goto fail; }
	if (bind_action_to_key("s",         1, INPUT_SORT_NEXT)                    == false) { goto fail; }
	if (bind_action_to_key("S",         1, INPUT_SORT_PREV)                    == false) { goto fail; }
	if (bind_action_to_key("u",         1, INPUT_TOGGLE_UNREAD_FIRST_SORTING)  == false) { goto fail; }
	if (bind_action_to_key("l",         1, INPUT_ENTER)                        == false) { goto fail; }
	if (bind_action_to_key("^J",        2, INPUT_ENTER)                        == false) { goto fail; }
	if (bind_action_to_key("KEY_RIGHT", 9, INPUT_ENTER)                        == false) { goto fail; }
	if (bind_action_to_key("KEY_ENTER", 9, INPUT_ENTER)                        == false) { goto fail; }
	if (bind_action_to_key("r",         1, INPUT_RELOAD)                       == false) { goto fail; }
	if (bind_action_to_key("^R",        2, INPUT_RELOAD_ALL)                   == false) { goto fail; }
	if (bind_action_to_key("d",         1, INPUT_MARK_READ_AND_JUMP_TO_NEXT)   == false) { goto fail; }
	if (bind_action_to_key("D",         1, INPUT_MARK_UNREAD_AND_JUMP_TO_NEXT) == false) { goto fail; }
	if (bind_action_to_key("^D",        2, INPUT_MARK_READ_ALL)                == false) { goto fail; }
	if (bind_action_to_key("i",         1, INPUT_MARK_IMPORTANT)               == false) { goto fail; }
	if (bind_action_to_key("I",         1, INPUT_MARK_UNIMPORTANT)             == false) { goto fail; }
	if (bind_action_to_key("e",         1, INPUT_TOGGLE_EXPLORE_MODE)          == false) { goto fail; }
	if (bind_action_to_key("v",         1, INPUT_STATUS_HISTORY_MENU)          == false) { goto fail; }
	if (bind_action_to_key("o",         1, INPUT_OPEN_IN_BROWSER)              == false) { goto fail; }
	if (bind_action_to_key("y",         1, INPUT_COPY_TO_CLIPBOARD)            == false) { goto fail; }
	if (bind_action_to_key("c",         1, INPUT_COPY_TO_CLIPBOARD)            == false) { goto fail; }
	if (bind_action_to_key("h",         1, INPUT_QUIT_SOFT)                    == false) { goto fail; }
	if (bind_action_to_key("q",         1, INPUT_QUIT_SOFT)                    == false) { goto fail; }
	if (bind_action_to_key("KEY_LEFT",  8, INPUT_QUIT_SOFT)                    == false) { goto fail; }
	if (bind_action_to_key("KEY_BACKSPACE", 13, INPUT_QUIT_SOFT)               == false) { goto fail; }
	if (bind_action_to_key("Q",         1, INPUT_QUIT_HARD)                    == false) { goto fail; }
	return true;
fail:
	fputs("Failed to assign default binds!\n", stderr);
	free_binds();
	return false;
}
