#include <stdio.h>
#include "newsraft.h"

bool
open_url_in_browser(const struct string *src)
{
	const struct string *browser_cmd = get_cfg_string(CFG_OPEN_IN_BROWSER_COMMAND);
	struct string *cmd = crtss(browser_cmd);
	if (cmd == NULL) {
		return false;
	}
	if (catcs(cmd, ' ') == false) {
		goto error;
	}
	if (catss(cmd, src) == false) {
		goto error;
	}
	system(cmd->ptr);
	free_string(cmd);
	return true;
error:
	free_string(cmd);
	return false;
}

bool
copy_string_to_clipboard(const struct string *src)
{
	const struct string *copy_cmd = get_cfg_string(CFG_COPY_TO_CLIPBOARD_COMMAND);
	FILE *p = popen(copy_cmd->ptr, "w");
	if (p == NULL) {
		return false;
	}
	fwrite(src->ptr, sizeof(char), src->len, p);
	pclose(p);
	return true;
}
