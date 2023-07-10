#include <string.h>
#include "load_config/load_config.h"

#define CONFIG_LINE_SIZE 1000

static inline bool
process_set_line(char *line)
{
	char *i;
	for (i = line; *i != '\0'; ++i) {
		if (ISWHITESPACE(*i)) {
			*i = '\0';
			i += 1;
			break;
		}
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
			fputs("Boolean settings only take \"true\" and \"false\" for values!\n", stderr);
			return false;
		}
	} else if (type == CFG_UINT) {
		size_t val;
		if ((*i == '\0') || (*i == '-') || (sscanf(i, "%zu", &val) != 1)) {
			fputs("Numeric settings only take non-negative integers for values!\n", stderr);
			return false;
		}
		set_cfg_uint(id, val);
	} else if (type == CFG_COLOR) {
		if (parse_color_setting(id, i) == false) {
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
process_bind_line(char *line)
{
	char *i;
	size_t key_len = 0;
	for (i = line; *i != '\0'; ++i) {
		if (ISWHITESPACE(*i)) {
			key_len = i - line;
			i += 1;
			break;
		}
	}
	if ((key_len == 5) && (memcmp(line, "space", 5) == 0)) {
		*line = ' ';
		key_len = 1;
	}
	ssize_t bind_index = create_empty_bind_or_clean_existing(line, key_len);
	if (bind_index < 0) {
		return false;
	}
	while (true) {
		while (ISWHITESPACE(*i)) {
			i += 1;
		}
		if (*i == '\0') break;
		char *border = strchr(i, ';');
		if (border != NULL) {
			if (border == i) {
				i += 1;
				continue;
			}
			*border = '\0';
		}
		if ((strncmp(i, "exec", 4) == 0) && (ISWHITESPACE(i[4]))) {
			i += 5;
			while (ISWHITESPACE(*i)) {
				i += 1;
			}
			if (attach_exec_action_to_bind(bind_index, i, strlen(i)) == false) {
				return false;
			}
		} else {
			input_cmd_id cmd = get_input_cmd_id_by_name(i);
			if (cmd == INPUT_ERROR || attach_cmd_action_to_bind(bind_index, cmd) == false) {
				return false;
			}
		}
		if (border == NULL) break;
		i = border + 1;
	}
	return true;
}

bool
parse_config_file(void)
{
	const char *config_path = get_config_path();
	if (config_path == NULL) {
		return true; // Since the config file is optional, don't return an error.
	}
	FILE *f = fopen(config_path, "r");
	if (f == NULL) {
		fputs("Couldn't open config file!\n", stderr);
		return false;
	}
	char type[6], line[CONFIG_LINE_SIZE + 1];
	size_t type_len, line_len;
	int c;
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
				goto invalid_format;
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
			if (process_bind_line(line) == false) {
				goto error;
			}
		} else if (strncmp(type, "unbind", type_len) == 0) {
			if (create_empty_bind_or_clean_existing(line, line_len) < 0) {
				goto error;
			}
		} else {
			goto invalid_format;
		}

	}
	fclose(f);
	return true;
invalid_format:
	fputs("Incorrect line notation! Lines can only start with either \"set\", \"bind\" or \"unbind\"!\n", stderr);
error:
	fputs("Failed to parse config file!\n", stderr);
	fclose(f);
	return false;
}
