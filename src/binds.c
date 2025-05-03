#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "newsraft.h"

#define INPUT_ARRAY
#include "input.h"
#undef INPUT_ARRAY

static struct input_binding *binds = NULL;

input_id
get_action_of_bind(struct input_binding *ctx, const char *key, size_t action_index, const struct wstring **p_arg)
{
	if (key != NULL) {
		struct input_binding *pool[] = {ctx, binds};
		for (int p = 0; p < 2; ++p) {
			for (struct input_binding *i = pool[p]; i != NULL; i = i->next) {
				if (strcmp(key, i->key->ptr) == 0) {
					if (action_index < i->actions_count) {
						*p_arg = i->actions[action_index].arg;
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
				free_wstring(i->actions[j].arg);
			}
			free(i->actions);
			i->actions = NULL;
			i->actions_count = 0;
			return i;
		}
	}
	struct input_binding *new = newsraft_calloc(1, sizeof(struct input_binding));
	new->key = crtas(key, strlen(key));
	new->next = *target;
	*target = new;
	return *target;
}

bool
attach_action_to_bind(struct input_binding *bind, input_id cmd, const char *arg, size_t arg_len)
{
	bind->actions = newsraft_realloc(bind->actions, sizeof(struct binding_action) * (bind->actions_count + 1));
	bind->actions[bind->actions_count].cmd = cmd;
	bind->actions[bind->actions_count].arg = NULL;
	if (arg && arg_len > 0) {
		bind->actions[bind->actions_count].arg = convert_array_to_wstring(arg, arg_len);
		if (bind->actions[bind->actions_count].arg == NULL) {
			return false;
		}
	}
	bind->actions_count += 1;
	INFO("Attached action: %14s, %zu, %2u, %s", bind->key->ptr, bind->actions_count, cmd, arg ? arg : "(none)");
	return true;
}

static bool
create2bind(const char *key, input_id action1, input_id action2)
{
	struct input_binding *bind = create_or_clean_bind(NULL, key);
	return attach_action_to_bind(bind, action1, NULL, 0) && attach_action_to_bind(bind, action2, NULL, 0);
}

bool
assign_default_binds(void)
{
	for (size_t i = 0; inputs[i].names[0] != NULL; ++i) {
		for (size_t j = 0; inputs[i].default_binds[j] != NULL; ++j) {
			struct input_binding *bind = create_or_clean_bind(NULL, inputs[i].default_binds[j]);
			if (!attach_action_to_bind(bind, i, NULL, 0)) {
				return false;
			}
		}
	}
	return create2bind("d", INPUT_MARK_READ, INPUT_JUMP_TO_NEXT) && create2bind("D", INPUT_MARK_UNREAD, INPUT_JUMP_TO_NEXT);
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
			free_wstring(i->actions[j].arg);
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
