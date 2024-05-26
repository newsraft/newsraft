#include <string.h>
#include "load_config/load_config.h"

#define CONFIG_ARRAY
#include "config.h"
#undef CONFIG_ARRAY

config_entry_id
find_config_entry_by_name(const char *name)
{
	for (config_entry_id i = 0; config[i].name != NULL; ++i) {
		if (strcmp(name, config[i].name) == 0) {
			return i;
		}
	}
	return CFG_ENTRIES_COUNT;
}

static struct config_entry *
get_global_or_context_config(struct config_context **ctx, config_entry_id id, bool create)
{
	if (ctx != NULL) {
		for (struct config_context *c = *ctx; c != NULL; c = c->next) {
			if (c->id == id) {
				return &c->cfg;
			}
		}
		if (create == true) {
			struct config_context *new = calloc(1, sizeof(struct config_context));
			new->id = id;
			new->cfg.type = get_cfg_type(id);
			new->next = *ctx;
			*ctx = new;
			return &new->cfg;
		}
	}
	return config + id;
}

config_type_id
get_cfg_type(config_entry_id id)
{
	return config[id].type;
}

bool
get_cfg_bool(struct config_context **ctx, config_entry_id id)
{
	return get_global_or_context_config(ctx, id, false)->value.b;
}

size_t
get_cfg_uint(struct config_context **ctx, config_entry_id id)
{
	return get_global_or_context_config(ctx, id, false)->value.u;
}

unsigned int
get_cfg_color(struct config_context **ctx, config_entry_id id)
{
	struct config_entry *cfg = get_global_or_context_config(ctx, id, false);
	if (cfg->value.c.color_pair >= 0 && arent_we_colorful()) {
		return COLOR_PAIR(cfg->value.c.color_pair) | cfg->value.c.attributes;
	}
	return A_NORMAL;
}

const struct string *
get_cfg_string(struct config_context **ctx, config_entry_id id)
{
	return get_global_or_context_config(ctx, id, false)->value.s.actual;
}

const struct wstring *
get_cfg_wstring(struct config_context **ctx, config_entry_id id)
{
	return get_global_or_context_config(ctx, id, false)->value.s.wactual;
}

void
set_cfg_bool(struct config_context **ctx, config_entry_id id, bool value)
{
	get_global_or_context_config(ctx, id, true)->value.b = value;
}

void
set_cfg_uint(struct config_context **ctx, config_entry_id id, size_t value)
{
	get_global_or_context_config(ctx, id, true)->value.u = value;
}

void
set_cfg_color(struct config_context **ctx, config_entry_id id, int fg, int bg, unsigned int attribute)
{
	static size_t color_pairs_count = 1; // Curses just wants us to start with 1
	struct config_entry *cfg = get_global_or_context_config(ctx, id, true);
	cfg->value.c.attributes = attribute;

	// For init_pair function to work you have to initialize curses first!
	if (init_pair(color_pairs_count, fg, bg) == OK) {
		cfg->value.c.color_pair = color_pairs_count;
		color_pairs_count += 1;
	} else {
		WARN("Failed to create color pair (%d, %d)", fg, bg);
		if (init_pair(color_pairs_count, -1, -1) == OK) {
			cfg->value.c.color_pair = color_pairs_count;
			color_pairs_count += 1;
		} else {
			WARN("Failed to create fallback color pair!");
		}
	}
}

bool
set_cfg_string(struct config_context **ctx, config_entry_id id, const char *src_ptr, size_t src_len)
{
	struct config_entry *cfg = get_global_or_context_config(ctx, id, true);
	if (cpyas(&cfg->value.s.actual, src_ptr, src_len) != true) {
		return false;
	}
	struct wstring *w = convert_string_to_wstring(cfg->value.s.actual);
	if (w == NULL) {
		write_error("Failed to convert %s setting value to wide characters!\n", config[id].name);
		return false;
	}
	bool status = wstr_set(&cfg->value.s.wactual, w->ptr, w->len, w->len);
	free_wstring(w);
	if (config[id].value.s.auto_set != NULL && strcmp(cfg->value.s.actual->ptr, "auto") == 0) {
		if (config[id].value.s.auto_set(ctx, id) == false) {
			write_error("Failed to set auto value for %s setting!\n", config[id].name);
			return false;
		}
	}
	return status;
}

static void
log_config_settings(void)
{
	INFO("Current configuration settings:");
	for (config_entry_id i = 0; config[i].name != NULL; ++i) {
		if (config[i].type == CFG_BOOL) {
			INFO("%s = %d", config[i].name, config[i].value.b);
		} else if (config[i].type == CFG_UINT) {
			INFO("%s = %zu", config[i].name, config[i].value.u);
		} else if (config[i].type == CFG_STRING) {
			INFO("%s = \"%s\"", config[i].name, config[i].value.s.actual->ptr);
		}
	}
}

void
free_config(void)
{
	INFO("Freeing configuration strings.");
	for (config_entry_id i = 0; config[i].name != NULL; ++i) {
		if (config[i].type == CFG_STRING) {
			free_string(config[i].value.s.actual);
			free_wstring(config[i].value.s.wactual);
		}
	}
}

void
free_config_context(struct config_context *ctx)
{
	for (struct config_context *c = ctx; c != NULL; ctx = c) {
		if (ctx->cfg.type == CFG_STRING) {
			free_string(ctx->cfg.value.s.actual);
			free_wstring(ctx->cfg.value.s.wactual);
		}
		c = ctx->next;
		free(ctx);
	}
}

bool
load_config(void)
{
	for (config_entry_id i = 0; config[i].name != NULL; ++i) {
		if (config[i].type == CFG_STRING && config[i].value.s.actual == NULL) {
			if (set_cfg_string(NULL, i, config[i].value.s.base, strlen(config[i].value.s.base)) == false) {
				write_error("Failed to prepare config settings!\n");
				free_config();
				return false;
			}
		} else if (config[i].type == CFG_COLOR && config[i].value.c.color_pair < 0) {
			set_cfg_color(NULL, i, config[i].value.c.fg, config[i].value.c.bg, config[i].value.c.attributes);
		}
	}
	log_config_settings();
	return true;
}
