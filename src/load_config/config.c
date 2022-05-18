#include "load_config/load_config.h"

enum config_type {
	CFG_BOOL,
	CFG_UINT,
	CFG_STRING,
	CFG_WSTRING,
};

union config_value {
	bool b;
	size_t u;
	struct string *s;
	struct wstring *w;
};

union config_default_string {
	const char *const s;
	const wchar_t *const w;
};

struct config_entry {
	const char *const name;
	const enum config_type type;
	union config_value value;
	const union config_default_string default_string;
	const size_t default_string_len;
};

static struct config_entry config[] = {
	{"max-items",                     CFG_UINT,    {.u = 0},     {.s = NULL},                                                       0},
	{"download-timeout",              CFG_UINT,    {.u = 20},    {.s = NULL},                                                       0},
	{"status-messages-limit",         CFG_UINT,    {.u = 0},     {.s = NULL},                                                       0},
	{"size-conversion-threshold",     CFG_UINT,    {.u = 1200},  {.s = NULL},                                                       0},
	{"menu-section-entry-format",     CFG_WSTRING, {.w = NULL},  {.w = L"%4.0u │ %t"},                                              10},
	{"menu-feed-entry-format",        CFG_WSTRING, {.w = NULL},  {.w = L"%4.0u │ %t"},                                              10},
	{"menu-item-entry-format",        CFG_WSTRING, {.w = NULL},  {.w = L" %u │ %t"},                                                8},
	{"proxy",                         CFG_STRING,  {.s = NULL},  {.s = ""},                                                         0},
	{"proxy-auth",                    CFG_STRING,  {.s = NULL},  {.s = ""},                                                         0},
	{"global-section-name",           CFG_STRING,  {.s = NULL},  {.s = "Global"},                                                   6},
	{"user-agent",                    CFG_STRING,  {.s = NULL},  {.s = ""}, /* generated later */                                   0},
	{"content-data-order",            CFG_STRING,  {.s = NULL},  {.s = "feed,title,authors,published,updated,max-summary-content"}, 56},
	{"content-date-format",           CFG_STRING,  {.s = NULL},  {.s = "%a, %d %b %Y %H:%M:%S %z"},                                 24},
	{"content-append-links",          CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"clean-database-on-startup",     CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"analyze-database-on-startup",   CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"send-user-agent-header",        CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"send-if-none-match-header",     CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"send-if-modified-since-header", CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"ssl-verify-host",               CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"ssl-verify-peer",               CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{NULL,                            CFG_BOOL,    {.b = false}, {.s = NULL},                                                       0},
};

static inline bool
assign_default_value_to_config_string(size_t i)
{
	config[i].value.s = crtas(config[i].default_string.s, config[i].default_string_len);
	if (config[i].value.s == NULL) {
		FAIL("Failed to assign \"%s\" to %s!", config[i].default_string.s, config[i].name);
		return false;
	}
	INFO("Assigned \"%s\" to %s.", config[i].default_string.s, config[i].name);
	return true;
}

static inline bool
assign_default_value_to_config_wstring(size_t i)
{
	config[i].value.w = wcrtas(config[i].default_string.w, config[i].default_string_len);
	if (config[i].value.w == NULL) {
		FAIL("Failed to assign \"%ls\" to %s!", config[i].default_string.w, config[i].name);
		return false;
	}
	INFO("Assigned \"%ls\" to %s.", config[i].default_string.w, config[i].name);
	return true;
}

bool
assign_default_values_to_null_config_strings(void)
{
	INFO("Assigning default values to NULL config strings.");
	for (size_t i = 0; config[i].name != NULL; ++i) {
		if (config[i].type == CFG_STRING) {
			if (config[i].value.s == NULL) {
				if (assign_default_value_to_config_string(i) == false) {
					return false;
				}
			}
		} else if (config[i].type == CFG_WSTRING) {
			if (config[i].value.w == NULL) {
				if (assign_default_value_to_config_wstring(i) == false) {
					return false;
				}
			}
		}
	}
	if (config[CFG_USER_AGENT].value.s->len == 0) {
		if (generate_useragent_string(config[CFG_USER_AGENT].value.s) == false) {
			return false;
		}
	}
	return true;
}

void
free_config(void)
{
	INFO("Freeing configuration strings.");
	for (size_t i = 0; config[i].name != NULL; ++i) {
		if (config[i].type == CFG_STRING) {
			free_string(config[i].value.s);
		} else if (config[i].type == CFG_WSTRING) {
			free_wstring(config[i].value.w);
		}
	}
	free_binds();
}

void
log_config_settings(void)
{
	INFO("Final configuration settings are:");
	for (size_t i = 0; config[i].name != NULL; ++i) {
		if (config[i].type == CFG_BOOL) {
			INFO("%s = %d", config[i].name, config[i].value.b);
		} else if (config[i].type == CFG_UINT) {
			INFO("%s = %zu", config[i].name, config[i].value.u);
		} else if (config[i].type == CFG_STRING) {
			INFO("%s = %s", config[i].name, config[i].value.s->ptr);
		} else if (config[i].type == CFG_WSTRING) {
			INFO("%s = %ls", config[i].name, config[i].value.w->ptr);
		}
	}
}

bool
get_cfg_bool(size_t i)
{
	return config[i].value.b;
}

size_t
get_cfg_uint(size_t i)
{
	return config[i].value.u;
}

const struct string *
get_cfg_string(size_t i)
{
	return config[i].value.s;
}

const struct wstring *
get_cfg_wstring(size_t i)
{
	return config[i].value.w;
}
