#include <string.h>
#include "load_config/load_config.h"

struct config_string {
	struct string *actual;
	struct wstring *wactual;
	const char *const base;
	bool (*auto_handler)(struct string **);
};

struct config_color {
	int fg;
	int bg;
	unsigned int attribute;
};

union config_value {
	bool b;
	size_t u;
	struct config_color c;
	struct config_string s;
};

struct config_entry {
	const char *const name;
	const config_type_id type;
	union config_value value;
};

static struct config_entry config[] = {
	{"color-status-good",               CFG_COLOR,  {.c = {COLOR_GREEN,   -1, A_NORMAL}}},
	{"color-status-info",               CFG_COLOR,  {.c = {COLOR_CYAN,    -1, A_NORMAL}}},
	{"color-status-fail",               CFG_COLOR,  {.c = {COLOR_RED,     -1, A_NORMAL}}},
	{"color-list-item",                 CFG_COLOR,  {.c = {-1,            -1, A_NORMAL}}},
	{"color-list-item-unread",          CFG_COLOR,  {.c = {COLOR_YELLOW,  -1, A_NORMAL}}},
	{"color-list-item-important",       CFG_COLOR,  {.c = {COLOR_MAGENTA, -1, A_NORMAL}}},
	{"color-list-feed",                 CFG_COLOR,  {.c = {-1,            -1, A_NORMAL}}},
	{"color-list-feed-unread",          CFG_COLOR,  {.c = {COLOR_YELLOW,  -1, A_NORMAL}}},
	{"color-list-section",              CFG_COLOR,  {.c = {-1,            -1, A_NORMAL}}},
	{"color-list-section-unread",       CFG_COLOR,  {.c = {COLOR_YELLOW,  -1, A_NORMAL}}},
	{"scrolloff",                       CFG_UINT,   {.u = 0   }},
	{"pager-width",                     CFG_UINT,   {.u = 0   }},
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
	{"item-content-format",             CFG_STRING, {.s = {NULL, NULL, "<b>Feed</b>:&nbsp;&nbsp;%f<br>|<b>Title</b>:&nbsp;%t<br>|<b>Date</b>:&nbsp;&nbsp;%d<br>|<br>%c<br>|<br><b>Links</b>:<br>%L", NULL}}},
	{"item-content-date-format",        CFG_STRING, {.s = {NULL, NULL, "%a, %d %b %Y %H:%M:%S %z",    NULL}}},
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
	{"mark-item-read-on-hover",         CFG_BOOL,   {.b = false}},
	{"analyze-database-on-startup",     CFG_BOOL,   {.b = true }},
	{"clean-database-on-startup",       CFG_BOOL,   {.b = false}},
	{"respect-ttl-element",             CFG_BOOL,   {.b = true }},
	{"respect-expires-header",          CFG_BOOL,   {.b = true }},
	{"send-user-agent-header",          CFG_BOOL,   {.b = true }},
	{"send-if-none-match-header",       CFG_BOOL,   {.b = true }},
	{"send-if-modified-since-header",   CFG_BOOL,   {.b = true }},
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

static inline bool
assign_values_to_null_config_strings(void)
{
	INFO("Assigning values to NULL config strings.");
	for (config_entry_id i = 0; config[i].name != NULL; ++i) {
		if (config[i].type == CFG_STRING && config[i].value.s.actual == NULL) {
			if (cpyas(&config[i].value.s.actual, config[i].value.s.base, strlen(config[i].value.s.base)) == false) {
				fprintf(stderr, "Not enough memory for %s string setting!\n", config[i].name);
				return false;
			}
		}
	}
	return true;
}

static inline bool
assign_values_to_auto_config_strings(void)
{
	INFO("Assigning values to auto config strings.");
	for (config_entry_id i = 0; config[i].name != NULL; ++i) {
		if ((config[i].type == CFG_STRING)
			&& (config[i].value.s.auto_handler != NULL)
			&& (strcmp(config[i].value.s.actual->ptr, "auto") == 0)
			&& (config[i].value.s.auto_handler(&config[i].value.s.actual) == false))
		{
			fprintf(stderr, "Failed to determine auto value for %s setting!\n", config[i].name);
			return false;
		}
	}
	return true;
}

static inline bool
assign_wide_values_to_config_strings(void)
{
	INFO("Assigning wstring members to config strings.");
	for (config_entry_id i = 0; config[i].name != NULL; ++i) {
		if (config[i].type == CFG_STRING) {
			config[i].value.s.wactual = convert_string_to_wstring(config[i].value.s.actual);
			if (config[i].value.s.wactual == NULL) {
				fprintf(stderr, "Failed to convert %s setting value to wide characters!\n", config[i].name);
				return false;
			}
		}
	}
	return true;
}

bool
prepare_config_string_settings(void)
{
	return assign_values_to_null_config_strings()
		&& assign_values_to_auto_config_strings()
		&& assign_wide_values_to_config_strings();
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

unsigned int
get_cfg_color(config_entry_id i, int *fg, int *bg)
{
	*fg = config[i].value.c.fg;
	*bg = config[i].value.c.bg;
	return config[i].value.c.attribute;
}

const struct string *
get_cfg_string(config_entry_id i)
{
	return config[i].value.s.actual;
}

const struct wstring *
get_cfg_wstring(config_entry_id i)
{
	return config[i].value.s.wactual;
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
set_cfg_color(config_entry_id i, int fg, int bg, unsigned int attribute)
{
	config[i].value.c.fg = fg;
	config[i].value.c.bg = bg;
	config[i].value.c.attribute = attribute;
}

bool
set_cfg_string(config_entry_id i, const char *src_ptr, size_t src_len)
{
	return cpyas(&config[i].value.s.actual, src_ptr, src_len);
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
