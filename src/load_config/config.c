#include "load_config/load_config.h"

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
	const config_type_id type;
	union config_value value;
	const union config_default_string default_string;
	const size_t default_string_len;
};

static struct config_entry config[] = {
	{"items-count-limit",               CFG_UINT,    {.u = 0},     {.s = NULL},                                                       0},
	{"download-timeout",                CFG_UINT,    {.u = 20},    {.s = NULL},                                                       0},
	{"download-speed-limit",            CFG_UINT,    {.u = 0},     {.s = NULL},                                                       0},
	{"status-messages-limit",           CFG_UINT,    {.u = 10000}, {.s = NULL},                                                       0},
	{"size-conversion-threshold",       CFG_UINT,    {.u = 1200},  {.s = NULL},                                                       0},
	{"copy-to-clipboard-command",       CFG_STRING,  {.s = NULL},  {.s = "auto"},                                                     4},
	{"proxy",                           CFG_STRING,  {.s = NULL},  {.s = ""},                                                         0},
	{"proxy-auth",                      CFG_STRING,  {.s = NULL},  {.s = ""},                                                         0},
	{"global-section-name",             CFG_STRING,  {.s = NULL},  {.s = "Global"},                                                   6},
	{"user-agent",                      CFG_STRING,  {.s = NULL},  {.s = "auto"},                                                     4},
	{"content-data-order",              CFG_STRING,  {.s = NULL},  {.s = "feed,title,authors,published,updated,max-summary-content"}, 56},
	{"content-date-format",             CFG_STRING,  {.s = NULL},  {.s = "%a, %d %b %Y %H:%M:%S %z"},                                 24},
	{"list-entry-date-format",          CFG_STRING,  {.s = NULL},  {.s = "%b %d"},                                                    5},
	{"menu-section-entry-format",       CFG_WSTRING, {.w = NULL},  {.w = L"%4.0u │ %t"},                                              10},
	{"menu-feed-entry-format",          CFG_WSTRING, {.w = NULL},  {.w = L"%4.0u │ %t"},                                              10},
	{"menu-item-entry-format",          CFG_WSTRING, {.w = NULL},  {.w = L" %u │ %d │ %t"},                                           13},
	{"menu-overview-item-entry-format", CFG_WSTRING, {.w = NULL},  {.w = L" %u │ %d │ %-28.28f │ %t"},                                24},
	{"mark-item-read-on-hover",         CFG_BOOL,    {.b = false}, {.s = NULL},                                                       0},
	{"content-append-links",            CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"clean-database-on-startup",       CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"analyze-database-on-startup",     CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"respect-ttl-element",             CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"respect-expires-header",          CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"send-user-agent-header",          CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"send-if-none-match-header",       CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"send-if-modified-since-header",   CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"ssl-verify-host",                 CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{"ssl-verify-peer",                 CFG_BOOL,    {.b = true},  {.s = NULL},                                                       0},
	{NULL,                              CFG_BOOL,    {.b = false}, {.s = NULL},                                                       0},
};

config_entry_id
find_config_entry_by_name(const char *name)
{
	for (size_t i = 0; config[i].name != NULL; ++i) {
		if (strcmp(name, config[i].name) == 0) {
			return i;
		}
	}
	return CFG_ENTRIES_COUNT;
}

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
	return true;
}

bool
assign_calculated_values_to_auto_config_strings(void)
{
	INFO("Assigning calculated values to auto config strings.");
	if (strcmp(config[CFG_USER_AGENT].value.s->ptr, "auto") == 0) {
		if (generate_useragent_string(config[CFG_USER_AGENT].value.s) == false) {
			return false;
		}
	}
	if (strcmp(config[CFG_COPY_TO_CLIPBOARD_COMMAND].value.s->ptr, "auto") == 0) {
		if (generate_copy_to_clipboard_command_string(config[CFG_COPY_TO_CLIPBOARD_COMMAND].value.s) == false) {
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

config_type_id
get_cfg_type(size_t i)
{
	return config[i].type;
}

const char *
get_cfg_name(size_t i)
{
	return config[i].name;
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

void
set_cfg_bool(size_t i, bool value)
{
	config[i].value.b = value;
}

void
set_cfg_uint(size_t i, size_t value)
{
	config[i].value.u = value;
}

bool
set_cfg_string(size_t i, const struct string *value)
{
	return crtss_or_cpyss(&config[i].value.s, value);
}

bool
set_cfg_wstring(size_t i, const struct string *value)
{
	struct wstring *wstr = convert_string_to_wstring(value);
	if (wstr == NULL) {
		return false;
	}
	if (config[i].value.w == NULL) {
		config[i].value.w = wstr;
	} else {
		if (wcpyss(config[i].value.w, wstr) == false) {
			free_wstring(wstr);
			return false;
		}
		free_wstring(wstr);
	}
	return true;
}
