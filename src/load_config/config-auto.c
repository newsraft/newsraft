#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include "load_config/load_config.h"

bool
obtain_useragent_string(struct config_context **ctx, config_type_id id)
{
	struct string *ua = NULL;
	if (cpyas(&ua, "newsraft/", 9) == false) {
		goto error;
	}
	if (catas(ua, NEWSRAFT_VERSION, strlen(NEWSRAFT_VERSION)) == false) {
		goto error;
	}
	struct utsname sys_data = {0};
	if (uname(&sys_data) >= 0 && strlen(sys_data.sysname) > 0) {
		if (catas(ua, " (", 2) == false) {
			goto error;
		}
		if (catas(ua, sys_data.sysname, strlen(sys_data.sysname)) == false) {
			goto error;
		}
		if (catcs(ua, ')') == false) {
			goto error;
		}
	}
	bool status = set_cfg_string(ctx, id, ua->ptr, ua->len);
	free_string(ua);
	return status;
error:
	write_error("Not enough memory for user-agent setting string!\n");
	free_string(ua);
	return false;
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
		return set_cfg_string(ctx, id, "false", 5);
	}
}

bool
obtain_notification_command(struct config_context **ctx, config_type_id id)
{
#ifdef __APPLE__
	return set_cfg_string(ctx, id, "osascript -e 'display notification \"Newsraft brought %q news!\"'", 63);
#endif
	return set_cfg_string(ctx, id, "notify-send 'Newsraft brought %q news!'", 39);
}
