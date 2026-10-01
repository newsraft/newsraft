#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "newsraft.h"

#define ACTIONS_IMPL
#include "actions.h"
#undef ACTIONS_IMPL

static struct input_binding *binds = NULL;
static bool was_escape_key_ever_bound = false;

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
	if (strcmp(key, "escape") == 0) {
		WARN("Escape key is used for key binding!");
		was_escape_key_ever_bound = true;
	}
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
bind_exec(const char *key, const char *cmd)
{
	struct input_binding *bind = create_or_clean_bind(NULL, key);
	return attach_action_to_bind(bind, INPUT_SYSTEM_COMMAND, cmd, strlen(cmd));
}

bool
assign_default_binds(void)
{
	struct {
		const char *keys[10];
		input_id actions[10];
	} default_binds[] = {
		{{"j", "KEY_DOWN", "^E"},      {INPUT_SELECT_NEXT}},
		{{"k", "KEY_UP", "^Y"},        {INPUT_SELECT_PREV}},
		{{"space", "^F", "KEY_NPAGE"}, {INPUT_SELECT_NEXT_PAGE}},
		{{"^D"},                       {INPUT_SELECT_NEXT_PAGE_HALF}},
		{{"^B", "KEY_PPAGE"},          {INPUT_SELECT_PREV_PAGE}},
		{{"^U"},                       {INPUT_SELECT_PREV_PAGE_HALF}},
		{{"g", "KEY_HOME"},            {INPUT_SELECT_FIRST}},
		{{"G", "KEY_END"},             {INPUT_SELECT_LAST}},
		{{"H"},                        {INPUT_TOGGLE_READ_MENU}},
		{{"J"},                        {INPUT_JUMP_TO_NEXT}},
		{{"K"},                        {INPUT_JUMP_TO_PREV}},
		{{"n"},                        {INPUT_JUMP_TO_NEXT_UNREAD}},
		{{"N"},                        {INPUT_JUMP_TO_PREV_UNREAD}},
		{{"p"},                        {INPUT_JUMP_TO_NEXT_IMPORTANT}},
		{{"P"},                        {INPUT_JUMP_TO_PREV_IMPORTANT}},
		{{"e"},                        {INPUT_JUMP_TO_NEXT_ERROR}},
		{{"E"},                        {INPUT_JUMP_TO_PREV_ERROR}},
		{{"*"},                        {INPUT_GOTO_FEED}},
		{{","},                        {INPUT_SHIFT_WEST}},
		{{"."},                        {INPUT_SHIFT_EAST}},
		{{"<"},                        {INPUT_SHIFT_RESET}},
		{{"t"},                        {INPUT_SORT_BY_TIME}},
		{{"w"},                        {INPUT_SORT_BY_ROWID}},
		{{"u"},                        {INPUT_SORT_BY_UNREAD}},
		{{"z"},                        {INPUT_SORT_BY_INITIAL}},
		{{"a"},                        {INPUT_SORT_BY_ALPHABET}},
		{{"i"},                        {INPUT_SORT_BY_IMPORTANT}},
		{{"l", "enter", "KEY_RIGHT", "KEY_ENTER"}, {INPUT_ENTER}},
		{{"r"},                        {INPUT_RELOAD}},
		{{"R", "^R"},                  {INPUT_RELOAD_ALL}},
		{{"A"},                        {INPUT_MARK_READ_ALL}},
		{{"f"},                        {INPUT_MARK_IMPORTANT}},
		{{"F"},                        {INPUT_MARK_UNIMPORTANT}},
		{{"tab"},                      {INPUT_TOGGLE_EXPLORE_MODE}},
		{{"v"},                        {INPUT_VIEW_ERRORS}},
		{{"o"},                        {INPUT_OPEN_IN_BROWSER}},
		{{"y", "c"},                   {INPUT_COPY_TO_CLIPBOARD}},
		{{"/"},                        {INPUT_START_SEARCH_INPUT}},
		{{"`"},                        {INPUT_CLEAN_STATUS}},
		{{"h", "backspace", "KEY_LEFT"}, {INPUT_NAVIGATE_BACK}},
		{{"q"},                        {INPUT_QUIT_SOFT}},
		{{"Q"},                        {INPUT_QUIT_HARD}},
		{{"d"},                        {INPUT_MARK_READ, INPUT_JUMP_TO_NEXT}},
		{{"D"},                        {INPUT_MARK_UNREAD, INPUT_JUMP_TO_NEXT}},
	};

	for (size_t i = 0; i < LENGTH(default_binds); ++i) {
		for (size_t j = 0; default_binds[i].keys[j] != NULL; ++j) {
			struct input_binding *bind = create_or_clean_bind(NULL, default_binds[i].keys[j]);
			for (size_t k = 0; default_binds[i].actions[k] != 0; ++k) {
				if (!attach_action_to_bind(bind, default_binds[i].actions[k], NULL, 0)) {
					return false;
				}
			}
		}
	}

	if (!bind_exec("?", "man newsraft")) {
		return false;
	}

	return true;
}

input_id
get_input_id_by_name(const char *name)
{
	for (size_t i = 0; LENGTH(g_actions); ++i) {
		for (size_t j = 0; g_actions[i].names[j] != NULL; ++j) {
			if (strcmp(name, g_actions[i].names[j]) == 0) {
				return i;
			}
		}
	}
	write_error("Action \"%s\" doesn't exist!\n", name);
	return INPUT_ERROR;
}

bool
is_escape_key_used(void)
{
	return was_escape_key_ever_bound;
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
