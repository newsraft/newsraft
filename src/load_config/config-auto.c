#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include "load_config/load_config.h"

bool
obtain_useragent_string(struct config_context **ctx, config_type_id id)
{
	struct string *ua = crtas("newsraft/", 9);
	catas(ua, NEWSRAFT_VERSION, strlen(NEWSRAFT_VERSION));
	struct utsname sys_data = {0};
	if (uname(&sys_data) >= 0 && strlen(sys_data.sysname) > 0) {
		catas(ua, " (", 2);
		catas(ua, sys_data.sysname, strlen(sys_data.sysname));
		catcs(ua, ')');
	}
	bool status = set_cfg_string(ctx, id, ua->ptr, ua->len);
	free_string(ua);
	return status;
}

bool
obtain_browser_command(struct config_context **ctx, config_type_id id)
{
#ifdef __APPLE__
	return set_cfg_string(ctx, id, "open \"%l\"", 9);
#endif
	return set_cfg_string(ctx, id, "${BROWSER:-xdg-open} \"%l\"", 25);
}

bool
obtain_clipboard_command(struct config_context **ctx, config_type_id id)
{
#ifdef __APPLE__
	return set_cfg_string(ctx, id, "pbcopy", 6);
#endif
	if (getenv("WAYLAND_DISPLAY") != NULL) {
		return set_cfg_string(ctx, id, "wl-copy", 7);
	} else if (getenv("DISPLAY") != NULL) {
		return set_cfg_string(ctx, id, "xclip -selection clipboard", 26);
	} else {
		return set_cfg_string(ctx, id, "newsraft-osc-52", 15);
	}
}

bool
obtain_notification_command(struct config_context **ctx, config_type_id id)
{
#ifdef __APPLE__
	return set_cfg_string(ctx, id, "osascript -e 'display notification \"Newsraft brought %q news!\"'", 63);
#endif
	if (getenv("WAYLAND_DISPLAY") != NULL || getenv("DISPLAY") != NULL) {
		return set_cfg_string(ctx, id, "notify-send 'Newsraft brought %q news!'", 39);
	} else {
		return set_cfg_string(ctx, id, "printf '\\e]9;Newsraft brought %q news!\\a'", 41);
	}
}
