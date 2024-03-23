#include <string.h>
#include "load_config/load_config.h"

struct config_string {
	struct string *actual;
	struct wstring *wactual;
	const char *const base;
	bool (*auto_handler)(struct config_context **, config_type_id);
};

struct config_color {
	int color_pair;
	int fg;
	int bg;
	unsigned int attributes;
};

union config_value {
	bool b;
	size_t u;
	struct config_color c;
	struct config_string s;
};

struct config_entry {
	const char *name;
	const config_type_id type;
	union config_value value;
};

struct config_context {
	config_entry_id id;
	struct config_entry cfg;
	struct config_context *next;
};

static struct config_entry config[] = {
	{"color-status-good",               CFG_COLOR,  {.c = {-1, COLOR_GREEN,   -1, A_NORMAL}}},
	{"color-status-info",               CFG_COLOR,  {.c = {-1, COLOR_CYAN,    -1, A_NORMAL}}},
	{"color-status-fail",               CFG_COLOR,  {.c = {-1, COLOR_RED,     -1, A_NORMAL}}},
	{"color-list-item",                 CFG_COLOR,  {.c = {-1, -1,            -1, A_NORMAL}}},
	{"color-list-item-unread",          CFG_COLOR,  {.c = {-1, COLOR_YELLOW,  -1, A_NORMAL}}},
	{"color-list-item-important",       CFG_COLOR,  {.c = {-1, COLOR_MAGENTA, -1, A_NORMAL}}},
	{"color-list-feed",                 CFG_COLOR,  {.c = {-1, -1,            -1, A_NORMAL}}},
	{"color-list-feed-unread",          CFG_COLOR,  {.c = {-1, COLOR_YELLOW,  -1, A_NORMAL}}},
	{"color-list-section",              CFG_COLOR,  {.c = {-1, -1,            -1, A_NORMAL}}},
	{"color-list-section-unread",       CFG_COLOR,  {.c = {-1, COLOR_YELLOW,  -1, A_NORMAL}}},
	{"reload-period",                   CFG_UINT,   {.u = 0   }},
	{"item-limit",                      CFG_UINT,   {.u = 0   }},
	{"scrolloff",                       CFG_UINT,   {.u = 0   }},
	{"pager-width",                     CFG_UINT,   {.u = 100 }},
	{"update-threads-count",            CFG_UINT,   {.u = 0   }},
	{"download-timeout",                CFG_UINT,   {.u = 20  }},
	{"download-speed-limit",            CFG_UINT,   {.u = 0   }},
	{"status-messages-count-limit",     CFG_UINT,   {.u = 1000}},
	{"copy-to-clipboard-command",       CFG_STRING, {.s = {NULL, NULL, "auto",   &obtain_clipboard_command}}},
	{"proxy",                           CFG_STRING, {.s = {NULL, NULL, "",       NULL}}},
	{"proxy-user",                      CFG_STRING, {.s = {NULL, NULL, "",       NULL}}},
	{"proxy-password",                  CFG_STRING, {.s = {NULL, NULL, "",       NULL}}},
	{"global-section-name",             CFG_STRING, {.s = {NULL, NULL, "Global", NULL}}},
	{"user-agent",                      CFG_STRING, {.s = {NULL, NULL, "auto",   &obtain_useragent_string}}},
	{"item-content-format",             CFG_STRING, {.s = {NULL, NULL, "<b>Feed</b>:&nbsp;&nbsp;%f<br>|<b>Title</b>:&nbsp;%t<br>|<b>Date</b>:&nbsp;&nbsp;%d<br>|<br>%c<br>|<br><hr>%L", NULL}}},
	{"item-content-date-format",        CFG_STRING, {.s = {NULL, NULL, "%a, %d %b %Y %H:%M:%S %z",    NULL}}},
	{"item-content-link-format",        CFG_STRING, {.s = {NULL, NULL, "<b>[%i]</b>:&nbsp;%l<br>",    NULL}}},
	{"list-entry-date-format",          CFG_STRING, {.s = {NULL, NULL, "%b %d",                       NULL}}},
	{"open-in-browser-command",         CFG_STRING, {.s = {NULL, NULL, "${BROWSER:-xdg-open} \"%l\"", NULL}}},
	{"notification-command",            CFG_STRING, {.s = {NULL, NULL, "auto",                        &obtain_notification_command}}},
	{"menu-section-entry-format",       CFG_STRING, {.s = {NULL, NULL, "%5.0u @ %t",                  NULL}}},
	{"menu-feed-entry-format",          CFG_STRING, {.s = {NULL, NULL, "%5.0u │ %t",                  NULL}}},
	{"menu-item-entry-format",          CFG_STRING, {.s = {NULL, NULL, " %u │ %d │ %o",               NULL}}},
	{"menu-explore-item-entry-format",  CFG_STRING, {.s = {NULL, NULL, " %u │ %d │ %-28O │ %o",       NULL}}},
	{"menu-feed-sorting",               CFG_STRING, {.s = {NULL, NULL, "none",                        NULL}}},
	{"menu-item-sorting",               CFG_STRING, {.s = {NULL, NULL, "time-desc",                   NULL}}},
	{"sections-menu-paramount-explore", CFG_BOOL,   {.b = false}},
	{"feeds-menu-paramount-explore",    CFG_BOOL,   {.b = false}},
	{"mark-item-unread-on-change",      CFG_BOOL,   {.b = true }},
	{"mark-item-read-on-hover",         CFG_BOOL,   {.b = false}},
	{"analyze-database-on-startup",     CFG_BOOL,   {.b = true }},
	{"clean-database-on-startup",       CFG_BOOL,   {.b = false}},
	{"respect-ttl-element",             CFG_BOOL,   {.b = true }},
	{"respect-expires-header",          CFG_BOOL,   {.b = true }},
	{"send-if-none-match-header",       CFG_BOOL,   {.b = true }},
	{"send-if-modified-since-header",   CFG_BOOL,   {.b = true }},
	{"pager-centering",                 CFG_BOOL,   {.b = true }},
	{NULL,                              CFG_BOOL,   {.b = false}},
};

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
	if (config[id].value.s.auto_handler != NULL && strcmp(cfg->value.s.actual->ptr, "auto") == 0) {
		if (config[id].value.s.auto_handler(ctx, id) == false) {
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
