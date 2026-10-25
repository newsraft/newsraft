#include <string.h>
#include "load_config/load_config.h"

static inline void
extract_token_from_line(struct string *line, struct string *token, bool break_on_whitespace)
{
	trim_whitespace_from_string(line);
	empty_string(token);
	size_t to_remove = 0;
	if (line->ptr[0] == '"' || line->ptr[0] == '\'') {
		const char *closing_quote = strchr(line->ptr + 1, line->ptr[0]);
		size_t token_len = closing_quote ? (size_t)(closing_quote - line->ptr) : strlen(line->ptr);
		catas(token, line->ptr + 1, token_len - 1);
		to_remove = token_len + 1;
	} else {
		to_remove = strcspn(line->ptr, break_on_whitespace ? "; \n\t\v\f\r" : ";");
		catas(token, line->ptr, to_remove);
	}
	remove_start_of_string(line, to_remove);
}

static bool
set_cfg_setting(struct config_context **ctx, config_entry_id id, const struct string *s)
{
	size_t val;
	const char *i = s->ptr;
	config_type_id type = get_cfg_type(id);
	switch (type) {
		case CFG_BOOL:
			if (*i != '\0' && strcmp(i, "true") != 0 && strcmp(i, "false") != 0) {
				write_error("Boolean settings only take \"true\" and \"false\" for values!\n");
				return false;
			}
			set_cfg_bool(ctx, id, *i == 'f' ? false : true);
			break;
		case CFG_UINT:
			if (*i == '\0' || *i == '-' || sscanf(i, "%zu", &val) != 1) {
				write_error("Numeric settings only take non-negative integers for values!\n");
				return false;
			}
			set_cfg_uint(ctx, id, val);
			break;
		case CFG_COLOR:
			return parse_color_setting(ctx, id, s->ptr);
		case CFG_STRING:
			return set_cfg_string(ctx, id, s->ptr, s->len);
	}
	return true;
}

bool
process_config_line(struct feed_entry *feed, const char *str, size_t len)
{
	struct string *line = crtas(str, len);
	struct string *token = crtes(200);
	struct config_context **cfg = feed != NULL ? &feed->cfg : NULL;
	struct input_binding **binds = feed != NULL ? &feed->binds : NULL;
	trim_whitespace_from_string(line);

	if (line->len > 0) {
		INFO("Config line %s -> %s", feed == NULL ? "" : feed->url->ptr, line->ptr);
	}

	struct input_binding *bind = NULL;

	while (line->len > 0) {
		while (line->ptr[0] == ';') {
			remove_start_of_string(line, 1);
		}
		trim_whitespace_from_string(line);
		if (line->len <= 0) break;
		if (line->ptr[0] == ';') continue;

		INFO("Config line remainder: %s", line->ptr);

		if (line->ptr[0] == '#') break; // Terminate on comments

		extract_token_from_line(line, token, true);

		if (strcmp(token->ptr, "set") == 0) {

			// set <setting> <value>

			extract_token_from_line(line, token, true);
			config_entry_id id = find_config_entry_by_name(token->ptr);
			if (id == CFG_ENTRIES_COUNT) {
				write_error("Setting \"%s\" doesn't exist!\n", token->ptr);
				goto error;
			}
			extract_token_from_line(line, token, false);
			if (set_cfg_setting(cfg, id, token) != true) {
				goto error;
			}
			continue;

		} else if (strcmp(token->ptr, "bind") == 0 || strcmp(token->ptr, "unbind") == 0) {

			// bind/unbind <key>

			extract_token_from_line(line, token, true);
			bind = bind_reset(binds, token->ptr);
			continue;

		} else if (find_config_entry_by_name(token->ptr) != CFG_ENTRIES_COUNT) {

			// <setting> <value>

			config_entry_id id = find_config_entry_by_name(token->ptr);
			extract_token_from_line(line, token, false);
			if (set_cfg_setting(cfg, id, token) != true) {
				goto error;
			}
			continue;

		} else if (bind != NULL) { // Takes previous bind entry into account

			input_id cmd = get_input_id_by_name(token->ptr);
			if (cmd == INPUT_ERROR) {
				goto error;
			}
			extract_token_from_line(line, token, false);
			if (!attach_action_to_bind(bind, cmd, token->ptr, token->len)) {
				goto error;
			}
			continue;

		}
		goto error;
	}
	free_string(line);
	free_string(token);
	return true;
error:
	write_error("Invalid config line: %s\n", str);
	write_error("Erroneous token: %s\n", token->ptr);
	free_string(line);
	free_string(token);
	return false;
}

bool
parse_config_file(void)
{
	const char *config_path = get_config_path();
	if (!config_path) {
		log_config_settings();
		return true; // Since a config file is optional, don't return the error.
	}
	FILE *f = fopen(config_path, "r");
	if (!f) {
		write_error("Couldn't open config file!\n");
		return false;
	}
	struct string *line = crtes(200);
	for (int c = '\n'; c != EOF; empty_string(line)) {
		for (c = fgetc(f); c != '\n' && c != EOF; c = fgetc(f)) {
			catcs(line, c);
		}
		if (!process_config_line(NULL, line->ptr, line->len)) {
			free_string(line);
			fclose(f);
			return false;
		}
	}
	free_string(line);
	fclose(f);
	log_config_settings();
	return true;
}
