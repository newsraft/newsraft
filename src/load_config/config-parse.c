#include <stdlib.h>
#include <stdio.h>
#include "load_config/load_config.h"

static inline void
replace_newline_with_terminator(char *line)
{
	char *newline_pos = strchr(line, '\n');
	if (newline_pos != NULL) {
		*newline_pos = '\0';
	}
}

static inline bool
process_set_line(char *line)
{
	struct string *setting_name = crtes();
	if (setting_name == NULL) {
		goto error;
	}
	char *i = line;
	do {
		if (catcs(setting_name, *i) == false) {
			free_string(setting_name);
			goto error;
		}
		i += 1;
		if (ISWHITESPACE(*i)) {
			break;
		}
	} while (*i != '\0');
	while (ISWHITESPACE(*i)) {
		i += 1;
	}
	config_entry_id id = find_config_entry_by_name(setting_name->ptr);
	if (id == CFG_ENTRIES_COUNT) {
		fprintf(stderr, "Setting \"%s\" doesn't exist!\n", setting_name->ptr);
		free_string(setting_name);
		return false;
	}
	free_string(setting_name);
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
		if ((strlen(i) == 0) || (sscanf(i, "%zu", &val) != 1)) {
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
	(void)line;
	fputs("\"bind\" lines are under construction!\n", stderr);
	return false;
}

static inline bool
process_config_file_line(char *line)
{
	INFO("Processing config line: \"%s\".", line);
	char *i = line;
	while (ISWHITESPACE(*i)) {
		i += 1;
	}
	if (*i == '\0') {
		return true; // Ignore empty lines.
	}
	// Just 5 characters because there are only 2 line types: "set" and "bind".
	char line_type[5];
	uint8_t line_type_len = 0;
	do {
		if (line_type_len == 4) {
			goto badline;
		}
		line_type[line_type_len++] = *i;
		i += 1;
		if (ISWHITESPACE(*i)) {
			break;
		}
	} while (*i != '\0');
	while (ISWHITESPACE(*i)) {
		i += 1;
	}
	if (*i == '\0') {
		INFO("This line is useless!");
		return true; // Ignore lines that don't do anything.
	}
	line_type[line_type_len] = '\0';
	if (strcmp(line_type, "set") == 0) {
		if (process_set_line(i) == false) {
			return false;
		}
	} else if (strcmp(line_type, "bind") == 0) {
		if (process_bind_line(i) == false) {
			return false;
		}
	} else {
		goto badline;
	}
	return true;
badline:
	fputs("Incorrect line notation! Lines can only start with either \"set\" or \"bind\"!\n", stderr);
	return false;
}

bool
parse_config_file(const char *path)
{
	FILE *f = fopen(path, "r");
	if (f == NULL) {
		fprintf(stderr, "Couldn't open config file!\n");
		return false;
	}

	char *line = NULL;
	size_t line_size = 0;
	size_t lines_count = 0;
	while (getline(&line, &line_size, f) != -1) {
		lines_count += 1;
		replace_newline_with_terminator(line);
		if (process_config_file_line(line) == false) {
			fprintf(stderr, "The error was detected on line %zu of config file.\n", lines_count);
			free(line);
			goto error;
		}
		free(line);
		line = NULL;
		line_size = 0;
	}

	if (line != NULL) {
		free(line);
	}

	fclose(f);
	return true;
error:
	fclose(f);
	return false;
}
