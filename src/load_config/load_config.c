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
			struct config_context *new = newsraft_calloc(1, sizeof(struct config_context));
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

struct config_color
get_cfg_color(struct config_context **ctx, config_entry_id id)
{
	struct config_entry *cfg = get_global_or_context_config(ctx, id, false);
	return cfg->value.c;
}

const struct string *
get_cfg_string(struct config_context **ctx, config_entry_id id)
{
	struct config_entry *cfg = get_global_or_context_config(ctx, id, false);
	if (cfg->value.s.actual == NULL) {
		set_cfg_string(NULL, id, config[id].value.s.base, strlen(config[id].value.s.base));
	}
	return cfg->value.s.actual;
}

const struct wstring *
get_cfg_wstring(struct config_context **ctx, config_entry_id id)
{
	struct config_entry *cfg = get_global_or_context_config(ctx, id, false);
	if (cfg->value.s.wactual == NULL) {
		set_cfg_string(NULL, id, config[id].value.s.base, strlen(config[id].value.s.base));
	}
	return cfg->value.s.wactual;
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
set_cfg_color(struct config_context **ctx, config_entry_id id, uintattr_t fg, uintattr_t bg, uintattr_t attribute)
{
	struct config_entry *cfg = get_global_or_context_config(ctx, id, true);
	cfg->value.c.fg = fg;
	cfg->value.c.bg = bg;
	cfg->value.c.attributes = attribute;
}

bool
set_cfg_string(struct config_context **ctx, config_entry_id id, const char *src_ptr, size_t src_len)
{
	struct config_entry *cfg = get_global_or_context_config(ctx, id, true);
	cpyas(&cfg->value.s.actual, src_ptr, src_len);
	struct wstring *w = convert_string_to_wstring(cfg->value.s.actual);
	if (w == NULL) {
		write_error("Failed to convert %s setting value to wide characters!\n", config[id].name);
		return false;
	}
	wstr_set(&cfg->value.s.wactual, w->ptr, w->len, w->len);
	free_wstring(w);
	if (config[id].value.s.auto_set != NULL && strcmp(cfg->value.s.actual->ptr, "auto") == 0) {
		if (config[id].value.s.auto_set(ctx, id) == false) {
			write_error("Failed to set auto value for %s setting!\n", config[id].name);
			return false;
		}
	}
	return true;
}

bool
is_cfg_color_set(struct config_context **ctx, config_entry_id id)
{
	struct config_entry *cfg = get_global_or_context_config(ctx, id, false);
	struct config_color *c = &cfg->value.c;
	return (
		!NEWSRAFT_ALL_BITS_SET(c->fg, uintattr_t)
		|| !NEWSRAFT_ALL_BITS_SET(c->bg, uintattr_t)
		|| !NEWSRAFT_ALL_BITS_SET(c->attributes, uintattr_t)
	);
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

void
log_config_settings(void)
{
	INFO("Current configuration settings:");
	for (config_entry_id i = 0; config[i].name != NULL; ++i) {
		if (config[i].type == CFG_BOOL) {
			INFO("%-35s = %d", config[i].name, config[i].value.b);
		} else if (config[i].type == CFG_UINT) {
			INFO("%-35s = %zu", config[i].name, config[i].value.u);
		} else if (config[i].type == CFG_STRING) {
			INFO("%-35s = \"%s\"", config[i].name, get_cfg_string(NULL, i)->ptr);
		}
	}
}
