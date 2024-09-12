#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "newsraft.h"

#define INPUT_ARRAY
#include "input.h"
#undef INPUT_ARRAY

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
	struct input_binding *new = calloc(1, sizeof(struct input_binding));
	struct string *new_key = crtas(key, strlen(key));
	if (new == NULL || new_key == NULL) {
		write_error("Not enough memory for binding!\n");
		free(new);
		free_string(new_key);
		return NULL;
	}
	new->key = new_key;
	new->next = *target;
	*target = new;
	return *target;
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
create2bind(const char *key, input_id action1, input_id action2)
{
	struct input_binding *bind = create_or_clean_bind(NULL, key);
	return bind && attach_action_to_bind(bind, action1) && attach_action_to_bind(bind, action2);
}

bool
assign_default_binds(void)
{
	for (size_t i = 0; inputs[i].names[0] != NULL; ++i) {
		for (size_t j = 0; inputs[i].default_binds[j] != NULL; ++j) {
			struct input_binding *bind = create_or_clean_bind(NULL, inputs[i].default_binds[j]);
			if (bind == NULL || attach_action_to_bind(bind, i) == false) {
				goto fail;
			}
		}
	}
	if (create2bind("d", INPUT_MARK_READ, INPUT_JUMP_TO_NEXT)   == false) { goto fail; }
	if (create2bind("D", INPUT_MARK_UNREAD, INPUT_JUMP_TO_NEXT) == false) { goto fail; }
	return true;
fail:
	write_error("Failed to assign default binds!\n");
	free_binds(binds);
	return false;
}

input_id
get_input_id_by_name(const char *name)
{
	for (size_t i = 0; inputs[i].names[0] != NULL; ++i) {
		for (size_t j = 0; inputs[i].names[j] != NULL; ++j) {
			if (strcmp(name, inputs[i].names[j]) == 0) {
				return i;
			}
		}
	}
	write_error("Action \"%s\" doesn't exist!\n", name);
	return INPUT_ERROR;
}

void
free_binds(struct input_binding *target)
{
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

void
free_default_binds(void)
{
	free_binds(binds);
}
