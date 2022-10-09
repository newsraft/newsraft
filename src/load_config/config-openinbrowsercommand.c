#include <stdlib.h>
#include <string.h>
#include "load_config/load_config.h"

bool
generate_open_in_browser_command_string(struct string *cmd)
{
	const char *env_browser = getenv("BROWSER");
	if (env_browser != NULL) {
		size_t env_browser_len = strlen(env_browser);
		if (env_browser_len != 0) {
			if (cpyas(cmd, env_browser, env_browser_len) == false) {
				goto error;
			}
			return true;
		}
	}
	if (cpyas(cmd, "xdg-open", 8) == false) {
		goto error;
	}
	return true;
error:
	fputs("Not enough memory for open-in-browser-command setting string!\n", stderr);
	return false;
}
