#include <string.h>
#include "load_config/load_config.h"

struct config_string {
	struct string *actual;
	const char *const primary;
	const size_t primary_len;
};

struct config_wstring {
	struct wstring *actual;
	const wchar_t *const primary;
	const size_t primary_len;
};

union config_value {
	bool b;
	size_t u;
	int c;
	struct config_string s;
	struct config_wstring w;
};

struct config_entry {
	const char *const name;
	const config_type_id type;
	union config_value value;
};

static struct config_entry config[] = {
	{"color-status-good-fg",            CFG_COLOR,   {.c = COLOR_GREEN  }},
	{"color-status-good-bg",            CFG_COLOR,   {.c = COLOR_BLACK  }},
	{"color-status-info-fg",            CFG_COLOR,   {.c = COLOR_CYAN   }},
	{"color-status-info-bg",            CFG_COLOR,   {.c = COLOR_BLACK  }},
	{"color-status-fail-fg",            CFG_COLOR,   {.c = COLOR_RED    }},
	{"color-status-fail-bg",            CFG_COLOR,   {.c = COLOR_BLACK  }},
	{"color-list-item-fg",              CFG_COLOR,   {.c = COLOR_WHITE  }},
	{"color-list-item-bg",              CFG_COLOR,   {.c = COLOR_BLACK  }},
	{"color-list-item-unread-fg",       CFG_COLOR,   {.c = COLOR_YELLOW }},
	{"color-list-item-unread-bg",       CFG_COLOR,   {.c = COLOR_BLACK  }},
	{"color-list-item-important-fg",    CFG_COLOR,   {.c = COLOR_MAGENTA}},
	{"color-list-item-important-bg",    CFG_COLOR,   {.c = COLOR_BLACK  }},
	{"color-list-feed-fg",              CFG_COLOR,   {.c = COLOR_WHITE  }},
	{"color-list-feed-bg",              CFG_COLOR,   {.c = COLOR_BLACK  }},
	{"color-list-feed-unread-fg",       CFG_COLOR,   {.c = COLOR_YELLOW }},
	{"color-list-feed-unread-bg",       CFG_COLOR,   {.c = COLOR_BLACK  }},
	{"color-list-section-fg",           CFG_COLOR,   {.c = COLOR_WHITE  }},
	{"color-list-section-bg",           CFG_COLOR,   {.c = COLOR_BLACK  }},
	{"color-list-section-unread-fg",    CFG_COLOR,   {.c = COLOR_YELLOW }},
	{"color-list-section-unread-bg",    CFG_COLOR,   {.c = COLOR_BLACK  }},
	{"items-count-limit",               CFG_UINT,    {.u = 0    }},
	{"download-timeout",                CFG_UINT,    {.u = 20   }},
	{"download-speed-limit",            CFG_UINT,    {.u = 0    }},
	{"status-messages-limit",           CFG_UINT,    {.u = 10000}},
	{"size-conversion-threshold",       CFG_UINT,    {.u = 1200 }},
	{"open-in-browser-command",         CFG_STRING,  {.s = {NULL, "auto",        4}}},
	{"copy-to-clipboard-command",       CFG_STRING,  {.s = {NULL, "auto",        4}}},
	{"proxy",                           CFG_STRING,  {.s = {NULL, "",            0}}},
	{"proxy-auth",                      CFG_STRING,  {.s = {NULL, "",            0}}},
	{"global-section-name",             CFG_STRING,  {.s = {NULL, "Global",      6}}},
	{"empty-title-placeholder",         CFG_STRING,  {.s = {NULL, "(untitled)", 10}}},
	{"user-agent",                      CFG_STRING,  {.s = {NULL, "auto",        4}}},
	{"item-formation-order",            CFG_STRING,  {.s = {NULL, "feed-url,title,authors,published,updated,max-content", 52}}},
	{"content-date-format",             CFG_STRING,  {.s = {NULL, "%a, %d %b %Y %H:%M:%S %z",  24}}},
	{"list-entry-date-format",          CFG_STRING,  {.s = {NULL, "%b %d",                      5}}},
	{"menu-section-entry-format",       CFG_WSTRING, {.w = {NULL, L"%4.0u @ %t",               10}}},
	{"menu-feed-entry-format",          CFG_WSTRING, {.w = {NULL, L"%4.0u │ %o",               10}}},
	{"menu-item-entry-format",          CFG_WSTRING, {.w = {NULL, L" %u │ %d │ %t",            13}}},
	{"menu-explore-item-entry-format",  CFG_WSTRING, {.w = {NULL, L" %u │ %d │ %-28.28o │ %t", 24}}},
	{"mark-item-read-on-hover",         CFG_BOOL,    {.b = false}},
	{"content-append-links",            CFG_BOOL,    {.b = true }},
	{"clean-database-on-startup",       CFG_BOOL,    {.b = true }},
	{"analyze-database-on-startup",     CFG_BOOL,    {.b = true }},
	{"respect-ttl-element",             CFG_BOOL,    {.b = true }},
	{"respect-expires-header",          CFG_BOOL,    {.b = true }},
	{"send-user-agent-header",          CFG_BOOL,    {.b = true }},
	{"send-if-none-match-header",       CFG_BOOL,    {.b = true }},
	{"send-if-modified-since-header",   CFG_BOOL,    {.b = true }},
	{"ssl-verify-host",                 CFG_BOOL,    {.b = true }},
	{"ssl-verify-peer",                 CFG_BOOL,    {.b = true }},
	{NULL,                              CFG_BOOL,    {.b = false}},
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
	config[i].value.s.actual = crtas(config[i].value.s.primary, config[i].value.s.primary_len);
	if (config[i].value.s.actual == NULL) {
		return false;
	}
	return true;
}

static inline bool
assign_default_value_to_config_wstring(size_t i)
{
	config[i].value.w.actual = wcrtas(config[i].value.w.primary, config[i].value.w.primary_len);
	if (config[i].value.w.actual == NULL) {
		return false;
	}
	return true;
}

bool
assign_default_values_to_null_config_strings(void)
{
	INFO("Assigning default values to NULL config strings.");
	for (size_t i = 0; config[i].name != NULL; ++i) {
		if (config[i].type == CFG_STRING) {
			if (config[i].value.s.actual == NULL) {
				if (assign_default_value_to_config_string(i) == false) {
					return false;
				}
			}
		} else if (config[i].type == CFG_WSTRING) {
			if (config[i].value.w.actual == NULL) {
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
	if (strcmp(config[CFG_USER_AGENT].value.s.actual->ptr, "auto") == 0) {
		if (generate_useragent_string(config[CFG_USER_AGENT].value.s.actual) == false) {
			return false;
		}
	}
	if (strcmp(config[CFG_OPEN_IN_BROWSER_COMMAND].value.s.actual->ptr, "auto") == 0) {
		if (generate_open_in_browser_command_string(config[CFG_OPEN_IN_BROWSER_COMMAND].value.s.actual) == false) {
			return false;
		}
	}
	if (strcmp(config[CFG_COPY_TO_CLIPBOARD_COMMAND].value.s.actual->ptr, "auto") == 0) {
		if (generate_copy_to_clipboard_command_string(config[CFG_COPY_TO_CLIPBOARD_COMMAND].value.s.actual) == false) {
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
			free_string(config[i].value.s.actual);
		} else if (config[i].type == CFG_WSTRING) {
			free_wstring(config[i].value.w.actual);
		}
	}
}

void
log_config_settings(void)
{
	INFO("Current configuration settings:");
	for (size_t i = 0; config[i].name != NULL; ++i) {
		if (config[i].type == CFG_BOOL) {
			INFO("%s = %d", config[i].name, config[i].value.b);
		} else if (config[i].type == CFG_UINT) {
			INFO("%s = %zu", config[i].name, config[i].value.u);
		} else if (config[i].type == CFG_STRING) {
			INFO("%s = \"%s\"", config[i].name, config[i].value.s.actual->ptr);
		} else if (config[i].type == CFG_WSTRING) {
			INFO("%s = \"%ls\"", config[i].name, config[i].value.w.actual->ptr);
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

int
get_cfg_color(size_t i)
{
	return config[i].value.c;
}

const struct string *
get_cfg_string(size_t i)
{
	return config[i].value.s.actual;
}

const struct wstring *
get_cfg_wstring(size_t i)
{
	return config[i].value.w.actual;
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

void
set_cfg_color(size_t i, int value)
{
	config[i].value.c = value;
}

bool
set_cfg_string(size_t i, const struct string *value)
{
	return crtss_or_cpyss(&config[i].value.s.actual, value);
}

bool
set_cfg_wstring(size_t i, const struct string *value)
{
	struct wstring *wstr = convert_string_to_wstring(value);
	if (wstr == NULL) {
		return false;
	}
	if (config[i].value.w.actual == NULL) {
		config[i].value.w.actual = wstr;
		return true;
	}
	if (wcpyss(config[i].value.w.actual, wstr) == false) {
		free_wstring(wstr);
		return false;
	}
	free_wstring(wstr);
	return true;
}
