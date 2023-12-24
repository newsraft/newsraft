#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include "load_config/load_config.h"

bool
obtain_useragent_string(struct string **ua)
{
	if (cpyas(ua, "newsraft/", 9) == false) {
		goto error;
	}
	if (catas(*ua, NEWSRAFT_VERSION, strlen(NEWSRAFT_VERSION)) == false) {
		goto error;
	}
	struct utsname sys_data;
	if (uname(&sys_data) == 0) {
		size_t sysname_len = strlen(sys_data.sysname);
		if (sysname_len != 0) {
			if (catas(*ua, " (", 2) == false) {
				goto error;
			}
			if (catas(*ua, sys_data.sysname, sysname_len) == false) {
				goto error;
			}
			if (catcs(*ua, ')') == false) {
				goto error;
			}
		}
	}
	return true;
error:
	fputs("Not enough memory for user-agent setting string!\n", stderr);
	return false;
}

bool
obtain_clipboard_command(struct string **cmd)
{
#ifdef __APPLE__
	return cpyas(cmd, "pbcopy", 6);
#endif
	if (getenv("WAYLAND_DISPLAY") != NULL) {
		return cpyas(cmd, "wl-copy", 7);
	} else if (getenv("DISPLAY") != NULL) {
		return cpyas(cmd, "xclip -selection clipboard", 26);
	} else {
		return cpyas(cmd, "false", 5);
	}
}

bool
obtain_notification_command(struct wstring **cmd)
{
#ifdef __APPLE__
	return wcpyas(cmd, L"osascript -e 'display notification \"Newsraft brought %q news!\"'", 63);
#endif
	return wcpyas(cmd, L"notify-send 'Newsraft brought %q news!'", 39);
}
