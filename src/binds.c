#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

struct binding_action {
	input_cmd_id cmd;
	struct wstring *exec;
};

struct input_binding {
	struct string *key;
	struct binding_action *actions;
	size_t actions_count;
};

static struct input_binding *binds = NULL;
static size_t binds_count = 0;

input_cmd_id
get_action_of_bind(int key_code, size_t action_index, const struct wstring **macro_ptr)
{
	const char *key_name = keyname(key_code);
	if (key_name != NULL) {
		for (size_t i = 0; i < binds_count; ++i) {
			if (strcmp(key_name, binds[i].key->ptr) == 0) {
				if (action_index < binds[i].actions_count) {
					*macro_ptr = binds[i].actions[action_index].exec;
					return binds[i].actions[action_index].cmd;
				}
				break;
			}
		}
	}
	return INPUT_ERROR;
}

ssize_t
create_empty_bind_or_clean_existing(const char *key_name, size_t key_name_len)
{
	for (size_t i = 0; i < binds_count; ++i) {
		if (key_name_len == binds[i].key->len && memcmp(key_name, binds[i].key->ptr, key_name_len) == 0) {
			for (size_t j = 0; j < binds[i].actions_count; ++j) {
				free_wstring(binds[i].actions[j].exec);
			}
			free(binds[i].actions);
			binds[i].actions = NULL;
			binds[i].actions_count = 0;
			return i;
		}
	}
	struct input_binding *tmp = realloc(binds, sizeof(struct input_binding) * (binds_count + 1));
	if (tmp != NULL) {
		binds = tmp;
		binds[binds_count].key = crtas(key_name, key_name_len);
		binds[binds_count].actions = NULL;
		binds[binds_count].actions_count = 0;
		if (binds[binds_count].key != NULL) {
			binds_count += 1;
			return binds_count - 1;
		}
	}
	fputs("Not enough memory for binding!\n", stderr);
	return -1;
}

static inline bool
attach_action_to_bind(ssize_t bind_index, input_cmd_id cmd, struct wstring *exec)
{
	struct binding_action *tmp = realloc(binds[bind_index].actions, sizeof(struct binding_action) * (binds[bind_index].actions_count + 1));
	if (tmp != NULL) {
		binds[bind_index].actions = tmp;
		binds[bind_index].actions[binds[bind_index].actions_count].cmd = cmd;
		binds[bind_index].actions[binds[bind_index].actions_count].exec = exec;
		binds[bind_index].actions_count += 1;
		INFO("Attached action: %14s, %zu, %2u, %ls", binds[bind_index].key->ptr, binds[bind_index].actions_count, cmd, exec == NULL ? L"none" : exec->ptr);
		return true;
	}
	fputs("Not enough memory for binding!\n", stderr);
	return false;
}

bool
attach_cmd_action_to_bind(ssize_t bind_index, input_cmd_id cmd_action)
{
	return attach_action_to_bind(bind_index, cmd_action, NULL);
}

bool
attach_exec_action_to_bind(ssize_t bind_index, const char *exec, size_t exec_len)
{
	struct wstring *wstr = convert_array_to_wstring(exec, exec_len);
	return wstr != NULL && attach_action_to_bind(bind_index, INPUT_SYSTEM_COMMAND, wstr);
}

static bool
create_bind(const char *key_name, size_t key_name_len, input_cmd_id cmd_action)
{
	const ssize_t bind_index = create_empty_bind_or_clean_existing(key_name, key_name_len);
	return bind_index >= 0 && attach_cmd_action_to_bind(bind_index, cmd_action) == true;
}

static bool
create_bind2(const char *key_name, size_t key_name_len, input_cmd_id cmd_action, input_cmd_id cmd_action2)
{
	const ssize_t bind_index = create_empty_bind_or_clean_existing(key_name, key_name_len);
	return bind_index >= 0
		&& attach_cmd_action_to_bind(bind_index, cmd_action) == true
		&& attach_cmd_action_to_bind(bind_index, cmd_action2) == true;
}

bool
assign_default_binds(void)
{
	if (create_bind("j",              1, INPUT_SELECT_NEXT)                     == false) { goto fail; }
	if (create_bind("KEY_DOWN",       8, INPUT_SELECT_NEXT) /* arrow down */    == false) { goto fail; }
	if (create_bind("^E",             2, INPUT_SELECT_NEXT) /* scroll down */   == false) { goto fail; }
	if (create_bind("k",              1, INPUT_SELECT_PREV)                     == false) { goto fail; }
	if (create_bind("KEY_UP",         6, INPUT_SELECT_PREV) /* arrow up */      == false) { goto fail; }
	if (create_bind("^Y",             2, INPUT_SELECT_PREV) /* scroll up */     == false) { goto fail; }
	if (create_bind(" ",              1, INPUT_SELECT_NEXT_PAGE)                == false) { goto fail; }
	if (create_bind("^F",             2, INPUT_SELECT_NEXT_PAGE)                == false) { goto fail; }
	if (create_bind("KEY_NPAGE",      9, INPUT_SELECT_NEXT_PAGE)                == false) { goto fail; }
	if (create_bind("^B",             2, INPUT_SELECT_PREV_PAGE)                == false) { goto fail; }
	if (create_bind("KEY_PPAGE",      9, INPUT_SELECT_PREV_PAGE)                == false) { goto fail; }
	if (create_bind("g",              1, INPUT_SELECT_FIRST)                    == false) { goto fail; }
	if (create_bind("KEY_HOME",       8, INPUT_SELECT_FIRST)                    == false) { goto fail; }
	if (create_bind("G",              1, INPUT_SELECT_LAST)                     == false) { goto fail; }
	if (create_bind("KEY_END",        7, INPUT_SELECT_LAST)                     == false) { goto fail; }
	if (create_bind("J",              1, INPUT_JUMP_TO_NEXT)                    == false) { goto fail; }
	if (create_bind("K",              1, INPUT_JUMP_TO_PREV)                    == false) { goto fail; }
	if (create_bind("n",              1, INPUT_JUMP_TO_NEXT_UNREAD)             == false) { goto fail; }
	if (create_bind("N",              1, INPUT_JUMP_TO_PREV_UNREAD)             == false) { goto fail; }
	if (create_bind("p",              1, INPUT_JUMP_TO_NEXT_IMPORTANT)          == false) { goto fail; }
	if (create_bind("P",              1, INPUT_JUMP_TO_PREV_IMPORTANT)          == false) { goto fail; }
	if (create_bind("s",              1, INPUT_SORT_NEXT)                       == false) { goto fail; }
	if (create_bind("S",              1, INPUT_SORT_PREV)                       == false) { goto fail; }
	if (create_bind("u",              1, INPUT_TOGGLE_UNREAD_FIRST_SORTING)     == false) { goto fail; }
	if (create_bind("l",              1, INPUT_ENTER)                           == false) { goto fail; }
	if (create_bind("^J",             2, INPUT_ENTER) /* actual enter key */    == false) { goto fail; }
	if (create_bind("KEY_RIGHT",      9, INPUT_ENTER) /* right arrow */         == false) { goto fail; }
	if (create_bind("KEY_ENTER",      9, INPUT_ENTER) /* sus enter key */       == false) { goto fail; }
	if (create_bind("r",              1, INPUT_RELOAD)                          == false) { goto fail; }
	if (create_bind("^R",             2, INPUT_RELOAD_ALL)                      == false) { goto fail; }
	if (create_bind2("d",             1, INPUT_MARK_READ, INPUT_JUMP_TO_NEXT)   == false) { goto fail; }
	if (create_bind2("D",             1, INPUT_MARK_UNREAD, INPUT_JUMP_TO_NEXT) == false) { goto fail; }
	if (create_bind("^D",             2, INPUT_MARK_READ_ALL)                   == false) { goto fail; }
	if (create_bind("i",              1, INPUT_MARK_IMPORTANT)                  == false) { goto fail; }
	if (create_bind("I",              1, INPUT_MARK_UNIMPORTANT)                == false) { goto fail; }
	if (create_bind("e",              1, INPUT_TOGGLE_EXPLORE_MODE)             == false) { goto fail; }
	if (create_bind("v",              1, INPUT_STATUS_HISTORY_MENU)             == false) { goto fail; }
	if (create_bind("o",              1, INPUT_OPEN_IN_BROWSER)                 == false) { goto fail; }
	if (create_bind("y",              1, INPUT_COPY_TO_CLIPBOARD)               == false) { goto fail; }
	if (create_bind("c",              1, INPUT_COPY_TO_CLIPBOARD)               == false) { goto fail; }
	if (create_bind("/",              1, INPUT_START_SEARCH_INPUT)              == false) { goto fail; }
	if (create_bind("h",              1, INPUT_NAVIGATE_BACK)                   == false) { goto fail; }
	if (create_bind("^?",             2, INPUT_NAVIGATE_BACK)                   == false) { goto fail; }
	if (create_bind("KEY_LEFT",       8, INPUT_NAVIGATE_BACK) /* left arrow */  == false) { goto fail; }
	if (create_bind("KEY_BACKSPACE", 13, INPUT_NAVIGATE_BACK)                   == false) { goto fail; }
	if (create_bind("q",              1, INPUT_QUIT_SOFT)                       == false) { goto fail; }
	if (create_bind("Q",              1, INPUT_QUIT_HARD)                       == false) { goto fail; }
	return true;
fail:
	fputs("Failed to assign default binds!\n", stderr);
	free_binds();
	return false;
}

void
free_binds(void)
{
	INFO("Freeing key binds.");
	for (size_t i = 0; i < binds_count; ++i) {
		free_string(binds[i].key);
		for (size_t j = 0; j < binds[i].actions_count; ++j) {
			free_wstring(binds[i].actions[j].exec);
		}
		free(binds[i].actions);
	}
	free(binds);
}
