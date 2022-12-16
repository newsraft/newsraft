#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include "load_config/load_config.h"

#define CONFIG_LINE_SIZE 1000

static inline bool
process_set_line(char *line)
{
	char *i = line;
	while (*i != '\0') {
		if (ISWHITESPACE(*i)) {
			*i = '\0';
			i += 1;
			break;
		}
		i += 1;
	}
	config_entry_id id = find_config_entry_by_name(line);
	if (id == CFG_ENTRIES_COUNT) {
		fprintf(stderr, "Setting \"%s\" doesn't exist!\n", line);
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
		if ((*i == '\0') || (strcmp(i, "true") == 0)) {
			set_cfg_bool(id, true);
		} else if (strcmp(i, "false") == 0) {
			set_cfg_bool(id, false);
		} else {
			fputs("Boolean settings can only take the values \"true\" or \"false\"!\n", stderr);
			return false;
		}
	} else if (type == CFG_UINT) {
		size_t val;
		if ((*i == '\0') || (*i == '-') || (sscanf(i, "%zu", &val) != 1)) {
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
	} else if (type == CFG_STRING) {
		if (set_cfg_string(id, i, strlen(i)) == false) {
			goto error;
		}
	} else if (type == CFG_WSTRING) {
		if (set_cfg_wstring(id, i, strlen(i)) == false) {
			goto error;
		}
	}
	return true;
error:
	fputs("Not enough memory for parsing config file!\n", stderr);
	return false;
}

static inline bool
process_bind_line(char *line, size_t line_len)
{
	char *i = line;
	size_t key_len = 0;
	while (*i != '\0') {
		if (ISWHITESPACE(*i)) {
			key_len = i - line;
			i += 1;
			break;
		}
		i += 1;
	}
	while (ISWHITESPACE(*i)) {
		i += 1;
	}
	if ((strncmp(i, "exec", 4) == 0) && (ISWHITESPACE(i[4]))) {
		i += 5;
		while (ISWHITESPACE(*i)) {
			i += 1;
		}
		return create_macro(line, key_len, i, line_len + line - i);
	} else {
		input_cmd_id cmd = get_input_cmd_id_by_name(i);
		if (cmd == INPUT_ERROR) {
			fprintf(stderr, "Action \"%s\" doesn't exist!\n", i);
			return false;
		}
		return assign_action_to_key(line, key_len, cmd);
	}
}

bool
parse_config_file(const char *path)
{
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		fputs("Couldn't open config file!\n", stderr);
		return false;
	}
	char type[6], line[CONFIG_LINE_SIZE + 1];
	size_t type_len, line_len;
	char c;
	// This is line-by-line file processing loop:
	// one iteration of loop results in one processed line.
	while (true) {

		// Get first non-whitespace character.
		do { c = fgetc(f); } while (ISWHITESPACE(c));

		if (c == '#') {
			// Skip a comment line.
			do { c = fgetc(f); } while (c != '\n' && c != EOF);
			continue;
		} else if (c == EOF) {
			break;
		}

		type_len = 0;
		do {
			if (type_len == 6) {
				goto invalid_line_type;
			}
			type[type_len++] = c;
			c = fgetc(f);
		} while (!ISWHITESPACE(c) && c != EOF);

		do { c = fgetc(f); } while (ISWHITESPACEEXCEPTNEWLINE(c));

		line_len = 0;
		while (c != '\n' && c != EOF) {
			if (line_len == CONFIG_LINE_SIZE) {
				fputs("Stumbled upon a config file line that is too long!\n", stderr);
				goto error;
			}
			line[line_len++] = c;
			c = fgetc(f);
		}
		// Delete trailing whitespace.
		while (line_len > 0 && ISWHITESPACE(line[line_len - 1])) {
			line_len -= 1;
		}
		line[line_len] = '\0';

		if (strncmp(type, "set", type_len) == 0) {
			if (process_set_line(line) == false) {
				goto error;
			}
		} else if (strncmp(type, "bind", type_len) == 0) {
			if (process_bind_line(line, line_len) == false) {
				goto error;
			}
		} else if (strncmp(type, "unbind", type_len) == 0) {
			delete_action_from_key(line);
		} else {
			goto invalid_line_type;
		}

	}
	fclose(f);
	return true;
invalid_line_type:
	fputs("Incorrect line notation! Lines can only start with either \"set\", \"bind\" or \"unbind\"!\n", stderr);
error:
	fclose(f);
	return false;
}
