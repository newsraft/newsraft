#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

struct input_macro {
	char *key;
	struct wstring *cmd;
};

static struct input_macro *macros = NULL;
static size_t macros_count = 0;

bool
create_macro(const char *key, size_t key_len, const char *cmd, size_t cmd_len)
{
	if ((key == NULL) || (key_len == 0) || (cmd == NULL) || (cmd_len == 0)) {
		return true; // Ignore empty macros.
	}
	struct input_macro *tmp = realloc(macros, sizeof(struct input_macro) * (macros_count + 1));
	if (tmp == NULL) {
		return false;
	}
	macros = tmp;
	size_t index = macros_count++;
	macros[index].key = malloc(sizeof(char) * (key_len + 1));
	if (macros[index].key == NULL) {
		return false;
	}
	memcpy(macros[index].key, key, sizeof(char) * key_len);
	macros[index].key[key_len] = '\0';
	struct string *cmd_str = crtas(cmd, cmd_len);
	if (cmd_str == NULL) {
		return false;
	}
	macros[index].cmd = convert_string_to_wstring(cmd_str);
	free_string(cmd_str);
	if (macros[index].cmd == NULL) {
		return false;
	}
	INFO("Added macro \"%ls\" to %s key.", macros[index].cmd->ptr, macros[index].key);
	return true;
}

const struct wstring *
find_macro(const char *key)
{
	for (size_t i = 0; i < macros_count; ++i) {
		if (strcmp(key, macros[i].key) == 0) {
			return macros[i].cmd;
		}
	}
	return NULL;
}

void
free_macros(void)
{
	while (macros_count > 0) {
		free(macros[macros_count - 1].key);
		free_wstring(macros[macros_count - 1].cmd);
		macros_count -= 1;
	}
	free(macros);
}
