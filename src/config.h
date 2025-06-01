#ifndef CONFIG_H
#define CONFIG_H

typedef uint8_t config_type_id;
enum config_type {
	CFG_BOOL,
	CFG_UINT,
	CFG_COLOR,
	CFG_STRING,
};

struct config_color {
	uintattr_t fg;
	uintattr_t bg;
	uintattr_t attributes;
};

struct config_string {
	const char *const base;
	struct string *actual;
	struct wstring *wactual;
	bool (*auto_set)(struct config_context **, config_type_id);
};

union config_value {
	bool b;
	size_t u;
	struct config_color c;
	struct config_string s;
};

struct config_entry {
	const char *name;
	config_type_id type;
	union config_value value;
};

struct config_context {
	config_entry_id id;
	struct config_entry cfg;
	struct config_context *next;
};

#define COLOR_TO_BIT(X) (1 << (X))

#define CFG(NAME, ...)  NAME,
enum {

#endif // CONFIG_H

#ifdef CONFIG_ARRAY

#define CFG(NAME, ...)  [NAME] = {__VA_ARGS__},
static struct config_entry config[] = {

#endif // CONFIG_ARRAY

CFG(CFG_COLOR_STATUS,                    "color-status",                    CFG_COLOR,  {.c = {TB_GREEN,   TB_DEFAULT, TB_BOLD}})
CFG(CFG_COLOR_STATUS_INFO,               "color-status-info",               CFG_COLOR,  {.c = {TB_CYAN,    TB_DEFAULT, TB_BOLD}})
CFG(CFG_COLOR_STATUS_FAIL,               "color-status-fail",               CFG_COLOR,  {.c = {TB_RED,     TB_DEFAULT, TB_BOLD}})
CFG(CFG_COLOR_LIST_ITEM,                 "color-list-item",                 CFG_COLOR,  {.c = {TB_DEFAULT, TB_DEFAULT, TB_DEFAULT}})
CFG(CFG_COLOR_LIST_ITEM_UNREAD,          "color-list-item-unread",          CFG_COLOR,  {.c = {TB_YELLOW,  TB_DEFAULT, TB_DEFAULT}})
CFG(CFG_COLOR_LIST_ITEM_IMPORTANT,       "color-list-item-important",       CFG_COLOR,  {.c = {TB_MAGENTA, TB_DEFAULT, TB_DEFAULT}})
CFG(CFG_COLOR_LIST_FEED,                 "color-list-feed",                 CFG_COLOR,  {.c = {TB_DEFAULT, TB_DEFAULT, TB_DEFAULT}})
CFG(CFG_COLOR_LIST_FEED_UNREAD,          "color-list-feed-unread",          CFG_COLOR,  {.c = {TB_YELLOW,  TB_DEFAULT, TB_DEFAULT}})
CFG(CFG_COLOR_LIST_FEED_FAILED,          "color-list-feed-failed",          CFG_COLOR,  {.c = {TB_RED,     TB_DEFAULT, TB_DEFAULT}})
CFG(CFG_COLOR_LIST_SECTION,              "color-list-section",              CFG_COLOR,  {.c = {TB_DEFAULT, TB_DEFAULT, TB_DEFAULT}})
CFG(CFG_COLOR_LIST_SECTION_UNREAD,       "color-list-section-unread",       CFG_COLOR,  {.c = {TB_YELLOW,  TB_DEFAULT, TB_DEFAULT}})
CFG(CFG_COLOR_LIST_SECTION_FAILED,       "color-list-section-failed",       CFG_COLOR,  {.c = {TB_RED,     TB_DEFAULT, TB_DEFAULT}})
CFG(CFG_RELOAD_PERIOD,                   "reload-period",                   CFG_UINT,   {.u = 0   })
CFG(CFG_ITEM_LIMIT,                      "item-limit",                      CFG_UINT,   {.u = 0   })
CFG(CFG_SCROLLOFF,                       "scrolloff",                       CFG_UINT,   {.u = 0   })
CFG(CFG_PAGER_WIDTH,                     "pager-width",                     CFG_UINT,   {.u = 100 })
CFG(CFG_DOWNLOAD_TIMEOUT,                "download-timeout",                CFG_UINT,   {.u = 20  })
CFG(CFG_DOWNLOAD_SPEED_LIMIT,            "download-speed-limit",            CFG_UINT,   {.u = 0   })
CFG(CFG_DOWNLOAD_MAX_HOST_CONNECTIONS,   "download-max-host-connections",   CFG_UINT,   {.u = 0   })
CFG(CFG_STATUS_PLACEHOLDER,              "status-placeholder",              CFG_STRING, {.s = {.base = "r:reload  R:reload-all  tab:explore  d:read  D:unread  f:important  F:unimportant  n:next-unread  N:prev-unread  p:next-important  P:prev-important"}})
CFG(CFG_COPY_TO_CLIPBOARD_COMMAND,       "copy-to-clipboard-command",       CFG_STRING, {.s = {.base = "auto", .auto_set = &obtain_clipboard_command}})
CFG(CFG_PROXY,                           "proxy",                           CFG_STRING, {.s = {.base = ""}})
CFG(CFG_PROXY_USER,                      "proxy-user",                      CFG_STRING, {.s = {.base = ""}})
CFG(CFG_PROXY_PASSWORD,                  "proxy-password",                  CFG_STRING, {.s = {.base = ""}})
CFG(CFG_GLOBAL_SECTION_NAME,             "global-section-name",             CFG_STRING, {.s = {.base = "Global"}})
CFG(CFG_USER_AGENT,                      "user-agent",                      CFG_STRING, {.s = {.base = "auto", .auto_set = &obtain_useragent_string}})
CFG(CFG_ITEM_RULE,                       "item-rule",                       CFG_STRING, {.s = {.base = ""}})
CFG(CFG_ITEM_CONTENT_FORMAT,             "item-content-format",             CFG_STRING, {.s = {.base = "<b>Feed</b>:&nbsp;&nbsp;%f<br>|<b>Title</b>:&nbsp;%t<br>|<b>Date</b>:&nbsp;&nbsp;%d<br>|<br>%c<br>|<br><hr>%L"}})
CFG(CFG_ITEM_CONTENT_DATE_FORMAT,        "item-content-date-format",        CFG_STRING, {.s = {.base = "%a, %d %b %Y %H:%M:%S %z"}})
CFG(CFG_ITEM_CONTENT_LINK_FORMAT,        "item-content-link-format",        CFG_STRING, {.s = {.base = "<b>[%i]</b>:&nbsp;%l<br>"}})
CFG(CFG_LIST_ENTRY_DATE_FORMAT,          "list-entry-date-format",          CFG_STRING, {.s = {.base = "%b %d"}})
CFG(CFG_OPEN_IN_BROWSER_COMMAND,         "open-in-browser-command",         CFG_STRING, {.s = {.base = "auto", .auto_set = &obtain_browser_command}})
CFG(CFG_NOTIFICATION_COMMAND,            "notification-command",            CFG_STRING, {.s = {.base = "auto", .auto_set = &obtain_notification_command}})
CFG(CFG_MENU_SECTION_ENTRY_FORMAT,       "menu-section-entry-format",       CFG_STRING, {.s = {.base = "%5.0u @ %t"}})
CFG(CFG_MENU_FEED_ENTRY_FORMAT,          "menu-feed-entry-format",          CFG_STRING, {.s = {.base = "%5.0u │ %t"}})
CFG(CFG_MENU_ITEM_ENTRY_FORMAT,          "menu-item-entry-format",          CFG_STRING, {.s = {.base = " %u │ %d │ %o"}})
CFG(CFG_MENU_EXPLORE_ITEM_ENTRY_FORMAT,  "menu-explore-item-entry-format",  CFG_STRING, {.s = {.base = " %u │ %d │ %-28O │ %o"}})
CFG(CFG_MENU_SECTION_SORTING,            "menu-section-sorting",            CFG_STRING, {.s = {.base = "none"}})
CFG(CFG_MENU_FEED_SORTING,               "menu-feed-sorting",               CFG_STRING, {.s = {.base = "none"}})
CFG(CFG_MENU_ITEM_SORTING,               "menu-item-sorting",               CFG_STRING, {.s = {.base = "time-desc"}})
CFG(CFG_SUPPRESS_ERRORS,                 "suppress-errors",                 CFG_BOOL,   {.b = false})
CFG(CFG_MENU_RESPONSIVENESS,             "menu-responsiveness",             CFG_BOOL,   {.b = true })
CFG(CFG_ITEM_LIMIT_UNREAD,               "item-limit-unread",               CFG_BOOL,   {.b = true })
CFG(CFG_ITEM_LIMIT_IMPORTANT,            "item-limit-important",            CFG_BOOL,   {.b = false})
CFG(CFG_STATUS_SHOW_MENU_PATH,           "status-show-menu-path",           CFG_BOOL,   {.b = true })
CFG(CFG_SECTIONS_MENU_PARAMOUNT_EXPLORE, "sections-menu-paramount-explore", CFG_BOOL,   {.b = false})
CFG(CFG_FEEDS_MENU_PARAMOUNT_EXPLORE,    "feeds-menu-paramount-explore",    CFG_BOOL,   {.b = false})
CFG(CFG_MARK_ITEM_UNREAD_ON_CHANGE,      "mark-item-unread-on-change",      CFG_BOOL,   {.b = false})
CFG(CFG_MARK_ITEM_READ_ON_HOVER,         "mark-item-read-on-hover",         CFG_BOOL,   {.b = false})
CFG(CFG_DATABASE_BATCH_TRANSACTIONS,     "database-batch-transactions",     CFG_BOOL,   {.b = true })
CFG(CFG_DATABASE_ANALYZE_ON_STARTUP,     "database-analyze-on-startup",     CFG_BOOL,   {.b = true })
CFG(CFG_DATABASE_CLEAN_ON_STARTUP,       "database-clean-on-startup",       CFG_BOOL,   {.b = false})
CFG(CFG_RESPECT_TTL_ELEMENT,             "respect-ttl-element",             CFG_BOOL,   {.b = true })
CFG(CFG_RESPECT_EXPIRES_HEADER,          "respect-expires-header",          CFG_BOOL,   {.b = true })
CFG(CFG_SEND_IF_NONE_MATCH_HEADER,       "send-if-none-match-header",       CFG_BOOL,   {.b = true })
CFG(CFG_SEND_IF_MODIFIED_SINCE_HEADER,   "send-if-modified-since-header",   CFG_BOOL,   {.b = true })
CFG(CFG_PAGER_CENTERING,                 "pager-centering",                 CFG_BOOL,   {.b = true })
CFG(CFG_IGNORE_NO_COLOR,                 "ignore-no-color",                 CFG_BOOL,   {.b = false})
CFG(CFG_ENTRIES_COUNT,                   NULL,                              CFG_BOOL,   {.b = false})

#ifdef CFG
};
#endif

#undef CFG
