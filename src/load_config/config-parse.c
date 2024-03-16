#include <string.h>
#include "load_config/load_config.h"

static inline void
extract_token_from_line(struct string *line, struct string *token, bool break_on_whitespace)
{
	trim_whitespace_from_string(line);
	empty_string(token);
	size_t to_remove = 0;
	if (line->ptr[0] == '"' || line->ptr[0] == '\'') {
		to_remove = 1;
		for (const char *i = line->ptr + 1; *i != '\0'; ++i) {
			to_remove += 1;
			if (*i == line->ptr[0]) break;
			catcs(token, *i);
		}
	} else {
		for (const char *i = line->ptr; *i != ';' && *i != '\0'; ++i) {
			if (break_on_whitespace == true && ISWHITESPACE(*i)) {
				break;
			}
			catcs(token, *i);
			to_remove += 1;
		}
	}
	remove_start_of_string(line, to_remove);
}

static bool
set_cfg_setting(struct config_context **ctx, config_entry_id id, const char *str, size_t len)
{
	struct string *s = crtas(str, len);
	const char *i = s->ptr;
	config_type_id type = get_cfg_type(id);
	switch (type) {
		case CFG_BOOL:
			if (*i != '\0' && strcmp(i, "true") != 0 && strcmp(i, "false") != 0) {
				fputs("Boolean settings only take \"true\" and \"false\" for values!\n", stderr);
				goto error;
			}
			set_cfg_bool(ctx, id, *i == 'f' ? false : true);
			break;
		case CFG_UINT:
			size_t val;
			if (*i == '\0' || *i == '-' || sscanf(i, "%zu", &val) != 1) {
				fputs("Numeric settings only take non-negative integers for values!\n", stderr);
				goto error;
			}
			set_cfg_uint(ctx, id, val);
			break;
		case CFG_COLOR:
			free_string(s);
			return parse_color_setting(ctx, id, i);
		case CFG_STRING:
			free_string(s);
			return set_cfg_string(ctx, id, str, len);
	}
	free_string(s);
	return true;
error:
	free_string(s);
	return false;
}

bool
process_config_line(struct feed_entry *feed, const char *str, size_t len)
{
	struct string *line = crtas(str, len);
	struct string *token = crtes(200);
	struct config_context **ctx = feed != NULL ? &feed->cfg : NULL;
	trim_whitespace_from_string(line);

	if (line->len > 0) {
		INFO("Config line %s -> %s", feed == NULL ? "" : feed->link->ptr, line->ptr);
	}

	ssize_t bind_index = -1;

	while (line->len > 0) {
		while (line->ptr[0] == ';') {
			remove_start_of_string(line, 1);
		}
		trim_whitespace_from_string(line);
		if (line->len <= 0) break;
		if (line->ptr[0] == ';') continue;

		INFO("Config line remainder: %s", line->ptr);

		if (line->ptr[0] == '#') {
			break; // Terminate on comments
		} else if (line->len > 3 && strncmp(line->ptr, "set", 3) == 0 && ISWHITESPACE(line->ptr[3])) {

			// set <setting> <value>

			remove_start_of_string(line, 3);
			extract_token_from_line(line, token, true);
			config_entry_id id = find_config_entry_by_name(token->ptr, token->len);
			if (id == CFG_ENTRIES_COUNT) {
				fprintf(stderr, "Setting \"%.*s\" doesn't exist!\n", (int)token->len, token->ptr);
				goto error;
			}
			extract_token_from_line(line, token, false);
			if (set_cfg_setting(ctx, id, token->ptr, token->len) != true) {
				goto error;
			}
			continue;

		} else if ((line->len > 4 && strncmp(line->ptr, "bind", 4) == 0 && ISWHITESPACE(line->ptr[4]))
			|| (line->len > 6 && strncmp(line->ptr, "unbind", 6) == 0 && ISWHITESPACE(line->ptr[6])))
		{
			// bind <key> <action1>

			remove_start_of_string(line, line->ptr[0] == 'b' ? 4 : 6);
			extract_token_from_line(line, token, true);
			if (token->len == 5 && memcmp(token->ptr, "space", 5) == 0) {
				bind_index = create_empty_bind_or_clean_existing(" ", 1);
			} else {
				bind_index = create_empty_bind_or_clean_existing(token->ptr, token->len);
			}
			continue;

		} else if (bind_index > 0) {
			// These will be parsed only taking into account the previous bind
			if (line->len > 4 && strncmp(line->ptr, "exec", 4) == 0 && ISWHITESPACE(line->ptr[4])) {

				remove_start_of_string(line, 4);
				extract_token_from_line(line, token, false);
				if (attach_exec_action_to_bind(bind_index, token->ptr, token->len) == false) {
					goto error;
				}

			} else {

				extract_token_from_line(line, token, true);
				input_cmd_id cmd = get_input_cmd_id_by_name(token->ptr, token->len);
				if (cmd == INPUT_ERROR || attach_cmd_action_to_bind(bind_index, cmd) == false) {
					goto error;
				}

			}
			continue;

		}
		goto error;
	}
	free_string(line);
	free_string(token);
	return true;
error:
	fprintf(stderr, "Invalid config line: %s", str);
	free_string(line);
	free_string(token);
	return false;
}

bool
parse_config_file(void)
{
	const char *config_path = get_config_path();
	if (config_path == NULL) {
		return true; // Since a config file is optional, don't return the error.
	}
	FILE *f = fopen(config_path, "r");
	if (f == NULL) {
		fputs("Couldn't open config file!\n", stderr);
		return false;
	}
	char *line = NULL;
	size_t size = 0;
	for (ssize_t len = getline(&line, &size, f); len >= 0; len = getline(&line, &size, f)) {
		if (process_config_line(NULL, line, len) == false) {
			free(line);
			fclose(f);
			return false;
		}
	}
	free(line);
	fclose(f);
	return true;
}
