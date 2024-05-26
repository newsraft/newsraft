#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

static struct input_binding *binds = NULL;

input_id
get_action_of_bind(struct input_binding *ctx, const char *key, size_t action_index, const struct wstring **macro_ptr)
{
	if (key != NULL) {
		struct input_binding *pool[] = {ctx, binds};
		for (int p = 0; p < 2; ++p) {
			for (struct input_binding *i = pool[p]; i != NULL; i = i->next) {
				if (strcmp(key, i->key->ptr) == 0) {
					if (action_index < i->actions_count) {
						*macro_ptr = i->actions[action_index].exec;
						return i->actions[action_index].cmd;
					}
					return INPUT_ERROR;
				}
			}
		}
	}
	return INPUT_ERROR;
}

struct input_binding *
create_or_clean_bind(struct input_binding **target, const char *key)
{
	if (target == NULL) {
		target = &binds;
	}
	if (strcasecmp(key, "space") == 0) {
		key = " ";
	} else if (strcasecmp(key, "tab") == 0) {
		key = "^I";
	} else if (strcasecmp(key, "enter") == 0) {
		key = "^J";
	} else if (strcasecmp(key, "escape") == 0) {
		key = "^[";
	} else if (strcasecmp(key, "backspace") == 0) {
		key = "^?";
	}
	for (struct input_binding *i = *target; i != NULL; i = i->next) {
		if (strcmp(key, i->key->ptr) == 0) {
			for (size_t j = 0; j < i->actions_count; ++j) {
				free_wstring(i->actions[j].exec);
			}
			free(i->actions);
			i->actions = NULL;
			i->actions_count = 0;
			return i;
		}
	}
	struct input_binding *new = malloc(sizeof(struct input_binding));
	if (new != NULL) {
		new->key = crtas(key, strlen(key));
		new->actions = NULL;
		new->actions_count = 0;
		new->next = *target;
		*target = new;
		return *target;
	}
	write_error("Not enough memory for binding!\n");
	return NULL;
}

static inline bool
attach_action_or_command_to_bind(struct input_binding *bind, input_id cmd, struct wstring *exec)
{
	struct binding_action *tmp = realloc(bind->actions, sizeof(struct binding_action) * (bind->actions_count + 1));
	if (tmp != NULL) {
		bind->actions = tmp;
		bind->actions[bind->actions_count].cmd = cmd;
		bind->actions[bind->actions_count].exec = exec;
		bind->actions_count += 1;
		INFO("Attached action: %14s, %zu, %2u, %ls", bind->key->ptr, bind->actions_count, cmd, exec == NULL ? L"none" : exec->ptr);
		return true;
	}
	write_error("Not enough memory for binding!\n");
	return false;
}

bool
attach_action_to_bind(struct input_binding *bind, input_id action)
{
	return attach_action_or_command_to_bind(bind, action, NULL);
}

bool
attach_command_to_bind(struct input_binding *bind, const char *exec, size_t exec_len)
{
	struct wstring *wstr = convert_array_to_wstring(exec, exec_len);
	return wstr != NULL && attach_action_or_command_to_bind(bind, INPUT_SYSTEM_COMMAND, wstr);
}

static bool
create1bind(const char *key, input_id action)
{
	struct input_binding *bind = create_or_clean_bind(NULL, key);
	return bind && attach_action_to_bind(bind, action);
}

static bool
create2bind(const char *key, input_id action1, input_id action2)
{
	struct input_binding *bind = create_or_clean_bind(NULL, key);
	return bind && attach_action_to_bind(bind, action1) && attach_action_to_bind(bind, action2);
}

bool
assign_default_binds(void)
{
	if (create1bind("j",             INPUT_SELECT_NEXT)                     == false) { goto fail; }
	if (create1bind("KEY_DOWN",      INPUT_SELECT_NEXT) /* arrow down */    == false) { goto fail; }
	if (create1bind("^E",            INPUT_SELECT_NEXT) /* scroll down */   == false) { goto fail; }
	if (create1bind("k",             INPUT_SELECT_PREV)                     == false) { goto fail; }
	if (create1bind("KEY_UP",        INPUT_SELECT_PREV) /* arrow up */      == false) { goto fail; }
	if (create1bind("^Y",            INPUT_SELECT_PREV) /* scroll up */     == false) { goto fail; }
	if (create1bind("space",         INPUT_SELECT_NEXT_PAGE)                == false) { goto fail; }
	if (create1bind("^F",            INPUT_SELECT_NEXT_PAGE)                == false) { goto fail; }
	if (create1bind("KEY_NPAGE",     INPUT_SELECT_NEXT_PAGE)                == false) { goto fail; }
	if (create1bind("^B",            INPUT_SELECT_PREV_PAGE)                == false) { goto fail; }
	if (create1bind("KEY_PPAGE",     INPUT_SELECT_PREV_PAGE)                == false) { goto fail; }
	if (create1bind("g",             INPUT_SELECT_FIRST)                    == false) { goto fail; }
	if (create1bind("KEY_HOME",      INPUT_SELECT_FIRST)                    == false) { goto fail; }
	if (create1bind("G",             INPUT_SELECT_LAST)                     == false) { goto fail; }
	if (create1bind("KEY_END",       INPUT_SELECT_LAST)                     == false) { goto fail; }
	if (create1bind("J",             INPUT_JUMP_TO_NEXT)                    == false) { goto fail; }
	if (create1bind("K",             INPUT_JUMP_TO_PREV)                    == false) { goto fail; }
	if (create1bind("n",             INPUT_JUMP_TO_NEXT_UNREAD)             == false) { goto fail; }
	if (create1bind("N",             INPUT_JUMP_TO_PREV_UNREAD)             == false) { goto fail; }
	if (create1bind("p",             INPUT_JUMP_TO_NEXT_IMPORTANT)          == false) { goto fail; }
	if (create1bind("P",             INPUT_JUMP_TO_PREV_IMPORTANT)          == false) { goto fail; }
	if (create1bind("*",             INPUT_GOTO_FEED)                       == false) { goto fail; }
	if (create1bind(",",             INPUT_SHIFT_WEST)                      == false) { goto fail; }
	if (create1bind(".",             INPUT_SHIFT_EAST)                      == false) { goto fail; }
	if (create1bind("<",             INPUT_SHIFT_RESET)                     == false) { goto fail; }
	if (create1bind("t",             INPUT_SORT_BY_TIME)                    == false) { goto fail; }
	if (create1bind("u",             INPUT_SORT_BY_UNREAD)                  == false) { goto fail; }
	if (create1bind("a",             INPUT_SORT_BY_ALPHABET)                == false) { goto fail; }
	if (create1bind("i",             INPUT_SORT_BY_IMPORTANT)               == false) { goto fail; }
	if (create1bind("l",             INPUT_ENTER)                           == false) { goto fail; }
	if (create1bind("enter",         INPUT_ENTER) /* actual enter key */    == false) { goto fail; }
	if (create1bind("KEY_RIGHT",     INPUT_ENTER) /* right arrow */         == false) { goto fail; }
	if (create1bind("KEY_ENTER",     INPUT_ENTER) /* sus enter key */       == false) { goto fail; }
	if (create1bind("r",             INPUT_RELOAD)                          == false) { goto fail; }
	if (create1bind("R",             INPUT_RELOAD_ALL)                      == false) { goto fail; }
	if (create1bind("^R",            INPUT_RELOAD_ALL)                      == false) { goto fail; }
	if (create2bind("d",             INPUT_MARK_READ, INPUT_JUMP_TO_NEXT)   == false) { goto fail; }
	if (create2bind("D",             INPUT_MARK_UNREAD, INPUT_JUMP_TO_NEXT) == false) { goto fail; }
	if (create1bind("^D",            INPUT_MARK_READ_ALL)                   == false) { goto fail; }
	if (create1bind("f",             INPUT_MARK_IMPORTANT)                  == false) { goto fail; }
	if (create1bind("F",             INPUT_MARK_UNIMPORTANT)                == false) { goto fail; }
	if (create1bind("tab",           INPUT_TOGGLE_EXPLORE_MODE)             == false) { goto fail; }
	if (create1bind("e",             INPUT_TOGGLE_EXPLORE_MODE)             == false) { goto fail; }
	if (create1bind("v",             INPUT_STATUS_HISTORY_MENU)             == false) { goto fail; }
	if (create1bind("o",             INPUT_OPEN_IN_BROWSER)                 == false) { goto fail; }
	if (create1bind("y",             INPUT_COPY_TO_CLIPBOARD)               == false) { goto fail; }
	if (create1bind("c",             INPUT_COPY_TO_CLIPBOARD)               == false) { goto fail; }
	if (create1bind("/",             INPUT_START_SEARCH_INPUT)              == false) { goto fail; }
	if (create1bind("h",             INPUT_NAVIGATE_BACK)                   == false) { goto fail; }
	if (create1bind("backspace",     INPUT_NAVIGATE_BACK)                   == false) { goto fail; }
	if (create1bind("KEY_LEFT",      INPUT_NAVIGATE_BACK) /* left arrow */  == false) { goto fail; }
	if (create1bind("KEY_BACKSPACE", INPUT_NAVIGATE_BACK)                   == false) { goto fail; }
	if (create1bind("q",             INPUT_QUIT_SOFT)                       == false) { goto fail; }
	if (create1bind("Q",             INPUT_QUIT_HARD)                       == false) { goto fail; }
	return true;
fail:
	write_error("Failed to assign default binds!\n");
	free_binds(NULL);
	return false;
}

void
free_binds(struct input_binding *target)
{
	if (target == NULL) {
		target = binds;
	}
	for (struct input_binding *i = target, *tmp = target; tmp != NULL; i = tmp) {
		free_string(i->key);
		for (size_t j = 0; j < i->actions_count; ++j) {
			free_wstring(i->actions[j].exec);
		}
		free(i->actions);
		tmp = i->next;
		free(i);
	}
}
