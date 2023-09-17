#include <stdlib.h>
#include "load_config/load_config.h"

bool
obtain_clipboard_command(struct string **cmd)
{
	if (getenv("WAYLAND_DISPLAY") != NULL) {
		if (cpyas(cmd, "wl-copy", 7) == false) {
			goto error;
		}
	} else if (getenv("DISPLAY") != NULL) {
		if (cpyas(cmd, "xclip -selection clipboard", 26) == false) {
			goto error;
		}
	} else {
		if (cpyas(cmd, "false", 5) == false) {
			goto error;
		}
	}
	return true;
error:
	fputs("Not enough memory for copy-to-clipboard-command setting string!\n", stderr);
	return false;
}
