#include <string.h>
#include "load_config/load_config.h"

struct config_string {
	struct string *actual;
	const char *const primary;
	const size_t primary_len;
	bool (*auto_handler)(struct string **);
};

struct config_wstring {
	struct wstring *actual;
	const wchar_t *const primary;
	const size_t primary_len;
};

struct config_color {
	int hue;
	unsigned int attribute;
};

union config_value {
	bool b;
	size_t u;
	struct config_color c;
	struct config_string s;
	struct config_wstring w;
};

struct config_entry {
	const char *const name;
	const config_type_id type;
	union config_value value;
};

static struct config_entry config[] = {
	{"color-status-good-fg",            CFG_COLOR,   {.c = {COLOR_GREEN,   A_NORMAL}}},
	{"color-status-good-bg",            CFG_COLOR,   {.c = {-1,            A_NORMAL}}},
	{"color-status-info-fg",            CFG_COLOR,   {.c = {COLOR_CYAN,    A_NORMAL}}},
	{"color-status-info-bg",            CFG_COLOR,   {.c = {-1,            A_NORMAL}}},
	{"color-status-fail-fg",            CFG_COLOR,   {.c = {COLOR_RED,     A_NORMAL}}},
	{"color-status-fail-bg",            CFG_COLOR,   {.c = {-1,            A_NORMAL}}},
	{"color-list-item-fg",              CFG_COLOR,   {.c = {-1,            A_NORMAL}}},
	{"color-list-item-bg",              CFG_COLOR,   {.c = {-1,            A_NORMAL}}},
	{"color-list-item-unread-fg",       CFG_COLOR,   {.c = {COLOR_YELLOW,  A_NORMAL}}},
	{"color-list-item-unread-bg",       CFG_COLOR,   {.c = {-1,            A_NORMAL}}},
	{"color-list-item-important-fg",    CFG_COLOR,   {.c = {COLOR_MAGENTA, A_NORMAL}}},
	{"color-list-item-important-bg",    CFG_COLOR,   {.c = {-1,            A_NORMAL}}},
	{"color-list-feed-fg",              CFG_COLOR,   {.c = {-1,            A_NORMAL}}},
	{"color-list-feed-bg",              CFG_COLOR,   {.c = {-1,            A_NORMAL}}},
	{"color-list-feed-unread-fg",       CFG_COLOR,   {.c = {COLOR_YELLOW,  A_NORMAL}}},
	{"color-list-feed-unread-bg",       CFG_COLOR,   {.c = {-1,            A_NORMAL}}},
	{"color-list-section-fg",           CFG_COLOR,   {.c = {-1,            A_NORMAL}}},
	{"color-list-section-bg",           CFG_COLOR,   {.c = {-1,            A_NORMAL}}},
	{"color-list-section-unread-fg",    CFG_COLOR,   {.c = {COLOR_YELLOW,  A_NORMAL}}},
	{"color-list-section-unread-bg",    CFG_COLOR,   {.c = {-1,            A_NORMAL}}},
	{"scrolloff",                       CFG_UINT,    {.u = 0   }},
	{"items-count-limit",               CFG_UINT,    {.u = 0   }},
	{"update-threads-count",            CFG_UINT,    {.u = 0   }},
	{"download-timeout",                CFG_UINT,    {.u = 10  }},
	{"download-speed-limit",            CFG_UINT,    {.u = 0   }},
	{"status-messages-count-limit",     CFG_UINT,    {.u = 1000}},
	{"copy-to-clipboard-command",       CFG_STRING,  {.s = {NULL, "auto",   4, &obtain_clipboard_command}}},
	{"proxy",                           CFG_STRING,  {.s = {NULL, "",       0, NULL}}},
	{"proxy-user",                      CFG_STRING,  {.s = {NULL, "",       0, NULL}}},
	{"proxy-password",                  CFG_STRING,  {.s = {NULL, "",       0, NULL}}},
	{"global-section-name",             CFG_STRING,  {.s = {NULL, "Global", 6, NULL}}},
	{"user-agent",                      CFG_STRING,  {.s = {NULL, "auto",   4, &obtain_useragent_string}}},
	{"item-formation-order",            CFG_STRING,  {.s = {NULL, "feed,title,authors,date,max-content", 35, NULL}}},
	{"content-date-format",             CFG_STRING,  {.s = {NULL, "%a, %d %b %Y %H:%M:%S %z",  24, NULL}}},
	{"list-entry-date-format",          CFG_STRING,  {.s = {NULL, "%b %d",                      5, NULL}}},
	{"open-in-browser-command",         CFG_WSTRING, {.w = {NULL, L"${BROWSER:-xdg-open} \"%l\"", 25}}},
	{"menu-section-entry-format",       CFG_WSTRING, {.w = {NULL, L"%5.0u @ %t",                  10}}},
	{"menu-feed-entry-format",          CFG_WSTRING, {.w = {NULL, L"%5.0u │ %o",                  10}}},
	{"menu-item-entry-format",          CFG_WSTRING, {.w = {NULL, L" %u │ %d │ %o",               13}}},
	{"menu-explore-item-entry-format",  CFG_WSTRING, {.w = {NULL, L" %u │ %d │ %-28O │ %o",       21}}},
	{"sections-menu-paramount-explore", CFG_BOOL,    {.b = false}},
	{"feeds-menu-paramount-explore",    CFG_BOOL,    {.b = false}},
	{"initial-unread-first-sorting",    CFG_BOOL,    {.b = false}},
	{"mark-item-read-on-hover",         CFG_BOOL,    {.b = false}},
	{"analyze-database-on-startup",     CFG_BOOL,    {.b = true }},
	{"clean-database-on-startup",       CFG_BOOL,    {.b = false}},
	{"respect-ttl-element",             CFG_BOOL,    {.b = true }},
	{"respect-expires-header",          CFG_BOOL,    {.b = true }},
	{"send-user-agent-header",          CFG_BOOL,    {.b = true }},
	{"send-if-none-match-header",       CFG_BOOL,    {.b = true }},
	{"send-if-modified-since-header",   CFG_BOOL,    {.b = true }},
	{NULL,                              CFG_BOOL,    {.b = false}},
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

bool
assign_default_values_to_null_config_strings(void)
{
	INFO("Assigning default values to NULL config strings.");
	for (config_entry_id i = 0; config[i].name != NULL; ++i) {
		if (config[i].type == CFG_STRING) {
			if (config[i].value.s.actual == NULL) {
				config[i].value.s.actual = crtas(config[i].value.s.primary, config[i].value.s.primary_len);
				if (config[i].value.s.actual == NULL) {
					return false;
				}
			}
		} else if (config[i].type == CFG_WSTRING) {
			if (config[i].value.w.actual == NULL) {
				config[i].value.w.actual = wcrtas(config[i].value.w.primary, config[i].value.w.primary_len);
				if (config[i].value.w.actual == NULL) {
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
	for (config_entry_id i = 0; config[i].name != NULL; ++i) {
		if ((config[i].type == CFG_STRING)
			&& (config[i].value.s.auto_handler != NULL)
			&& (strcmp(config[i].value.s.actual->ptr, "auto") == 0)
			&& (config[i].value.s.auto_handler(&config[i].value.s.actual) == false))
		{
			return false;
		}
	}
	return true;
}

config_type_id
get_cfg_type(config_entry_id i)
{
	return config[i].type;
}

bool
get_cfg_bool(config_entry_id i)
{
	return config[i].value.b;
}

size_t
get_cfg_uint(config_entry_id i)
{
	return config[i].value.u;
}

int
get_cfg_color(config_entry_id i, unsigned int *attribute)
{
	*attribute = config[i].value.c.attribute;
	return config[i].value.c.hue;
}

const struct string *
get_cfg_string(config_entry_id i)
{
	return config[i].value.s.actual;
}

const struct wstring *
get_cfg_wstring(config_entry_id i)
{
	return config[i].value.w.actual;
}

void
set_cfg_bool(config_entry_id i, bool value)
{
	config[i].value.b = value;
}

void
set_cfg_uint(config_entry_id i, size_t value)
{
	config[i].value.u = value;
}

void
set_cfg_color_hue(config_entry_id i, int hue)
{
	config[i].value.c.hue = hue;
}

void
set_cfg_color_attribute(config_entry_id i, unsigned int attribute)
{
	config[i].value.c.attribute = attribute;
}

bool
set_cfg_string(config_entry_id i, const char *src_ptr, size_t src_len)
{
	return cpyas(&config[i].value.s.actual, src_ptr, src_len);
}

bool
set_cfg_wstring(config_entry_id i, const char *src_ptr, size_t src_len)
{
	struct wstring *wstr = convert_array_to_wstring(src_ptr, src_len);
	if (wstr == NULL) {
		return false;
	}
	free_wstring(config[i].value.w.actual);
	config[i].value.w.actual = wstr;
	return true;
}

void
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
		} else if (config[i].type == CFG_WSTRING) {
			INFO("%s = \"%ls\"", config[i].name, config[i].value.w.actual->ptr);
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
		} else if (config[i].type == CFG_WSTRING) {
			free_wstring(config[i].value.w.actual);
		}
	}
}
