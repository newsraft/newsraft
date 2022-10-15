#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include "load_config/load_config.h"

static inline void
remove_trailing_whitespace(char *line)
{
	char *i = line;
	while (*i != '\0') {
		i += 1;
	}
	if (i != line) {
		i -= 1;
	}
	while (ISWHITESPACE(*i)) {
		*i = '\0';
		if (i == line) {
			break;
		}
		i -= 1;
	}
}

static inline bool
process_set_line(char *line)
{
	char setting_name[100];
	uint8_t setting_name_len = 0;
	char *i = line;
	while (*i != '\0') {
		if (ISWHITESPACE(*i)) {
			break;
		}
		if (setting_name_len > 90) {
			fputs("This setting name exceeds the maximum possible length of valid setting!\n", stderr);
			return false;
		}
		setting_name[setting_name_len++] = *i;
		i += 1;
	}
	setting_name[setting_name_len] = '\0';
	config_entry_id id = find_config_entry_by_name(setting_name);
	if (id == CFG_ENTRIES_COUNT) {
		fprintf(stderr, "Setting \"%s\" doesn't exist!\n", setting_name);
		return false;
	}
	while (ISWHITESPACE(*i)) {
		i += 1;
	}
	if (*i == '"') {
		char *next_double_quote = strchr(i + 1, '"');
		if (next_double_quote != NULL) {
			*next_double_quote = '\0';
			i += 1;
		}
	} else if (*i == '\'') {
		char *next_single_quote = strchr(i + 1, '\'');
		if (next_single_quote != NULL) {
			*next_single_quote = '\0';
			i += 1;
		}
	}
	config_type_id type = get_cfg_type(id);
	if (type == CFG_BOOL) {
		if ((strlen(i) == 0) || (strcmp(i, "true") == 0)) {
			set_cfg_bool(id, true);
		} else if (strcmp(i, "false") == 0) {
			set_cfg_bool(id, false);
		} else {
			fputs("Boolean settings can only take the values \"true\" or \"false\"!\n", stderr);
			return false;
		}
	} else if (type == CFG_UINT) {
		size_t val;
		if ((strlen(i) == 0) || (*i == '-') || (sscanf(i, "%zu", &val) != 1)) {
			fputs("Numeric settings can only take non-negative integer values!\n", stderr);
			return false;
		}
		set_cfg_uint(id, val);
	} else if (type == CFG_COLOR) {
		if (strcmp(i, "black") == 0) {
			set_cfg_color(id, COLOR_BLACK);
		} else if (strcmp(i, "red") == 0) {
			set_cfg_color(id, COLOR_RED);
		} else if (strcmp(i, "green") == 0) {
			set_cfg_color(id, COLOR_GREEN);
		} else if (strcmp(i, "yellow") == 0) {
			set_cfg_color(id, COLOR_YELLOW);
		} else if (strcmp(i, "blue") == 0) {
			set_cfg_color(id, COLOR_BLUE);
		} else if (strcmp(i, "magenta") == 0) {
			set_cfg_color(id, COLOR_MAGENTA);
		} else if (strcmp(i, "cyan") == 0) {
			set_cfg_color(id, COLOR_CYAN);
		} else if (strcmp(i, "white") == 0) {
			set_cfg_color(id, COLOR_WHITE);
		} else {
			fputs("Color settings can only take lower-case ASCII color names!\n", stderr);
			return false;
		}
	} else if ((type == CFG_STRING) || (type == CFG_WSTRING)) {
		struct string *str = crtas(i, strlen(i));
		if (str == NULL) {
			goto error;
		}
		bool success;
		if (type == CFG_STRING) {
			success = set_cfg_string(id, str);
		} else {
			success = set_cfg_wstring(id, str);
		}
		free_string(str);
		if (success == false) {
			goto error;
		}
	}
	return true;
error:
	fputs("Not enough memory for parsing config file!\n", stderr);
	return false;
}

static inline bool
process_bind_line(char *line)
{
	char key_name[20];
	uint8_t key_name_len = 0;
	char *i = line;
	while (*i != '\0') {
		if (ISWHITESPACE(*i)) {
			break;
		}
		if (key_name_len > 15) {
			fputs("This key name exceeds the maximum possible length of valid key!\n", stderr);
			return false;
		}
		key_name[key_name_len++] = *i;
		i += 1;
	}
	key_name[key_name_len] = '\0';
	while (ISWHITESPACE(*i)) {
		i += 1;
	}
	if ((strncmp(i, "exec", 4) == 0) && (ISWHITESPACE(i[4]))) {
		i += 5;
		while (ISWHITESPACE(*i)) {
			i += 1;
		}
		return create_macro(key_name, key_name_len, i, strlen(i));
	} else {
		input_cmd_id cmd = get_input_cmd_id_by_name(i);
		if (cmd == INPUTS_COUNT) {
			fprintf(stderr, "Action \"%s\" doesn't exist!\n", i);
			return false;
		}
		return assign_action_to_key(key_name, key_name_len, cmd);
	}
}

static inline bool
process_config_file_line(char *line)
{
	remove_trailing_whitespace(line);
	INFO("Processing config line: %s", line);
	char *i = line;
	while (ISWHITESPACE(*i)) {
		i += 1;
	}
	if ((*i == '#') || (*i == '\0')) {
		return true; // Ignore comments and empty lines.
	}
	if ((strncmp(i, "set", 3) == 0) && ISWHITESPACE(*(i + 3))) {
		i += 4;
		while (ISWHITESPACE(*i)) {
			i += 1;
		}
		return process_set_line(i);
	} else if ((strncmp(i, "bind", 4) == 0) && ISWHITESPACE(*(i + 4))) {
		i += 5;
		while (ISWHITESPACE(*i)) {
			i += 1;
		}
		return process_bind_line(i);
	} else if ((strncmp(i, "unbind", 6) == 0) && ISWHITESPACE(*(i + 6))) {
		i += 7;
		while (ISWHITESPACE(*i)) {
			i += 1;
		}
		// Since we deleted trailing whitespace from the line, we can pass its
		// remainings as a key to delete an action from.
		delete_action_from_key(i);
		return true;
	}
	fputs("Incorrect line notation! ", stderr);
	fputs("Lines can only start with either \"set\", \"bind\" or \"unbind\"!\n", stderr);
	return false;
}

bool
parse_config_file(const char *path)
{
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		fputs("Couldn't open config file!\n", stderr);
		return false;
	}
	char *line = NULL;
	size_t line_size = 0;
	size_t lines_count = 0;
	while (getline(&line, &line_size, f) != -1) {
		lines_count += 1;
		if (process_config_file_line(line) == false) {
			fprintf(stderr, "The error was detected on line %zu of config file.\n", lines_count);
			free(line);
			fclose(f);
			return false;
		}
	}
	free(line);
	fclose(f);
	return true;
}
