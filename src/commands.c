#include <stdlib.h>
#include <stdio.h>
#include "newsraft.h"

// Note to the future: these functions only return true if something actually
// happened, because we need the boolean result of thesse functions as an
// indication whether the screen has to be redrawn or not.

bool
open_url_in_browser(const struct string *src)
{
	if ((src == NULL) || (src->len == 0)) {
		return false;
	}
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
	if ((src == NULL) || (src->len == 0)) {
		return false;
	}
	const struct string *copy_cmd = get_cfg_string(CFG_COPY_TO_CLIPBOARD_COMMAND);
	FILE *p = popen(copy_cmd->ptr, "w");
	if (p == NULL) {
		fail_status("Failed to copy %s", src->ptr);
		return false;
	}
	fwrite(src->ptr, sizeof(char), src->len, p);
	pclose(p);
	good_status("Copied %s", src->ptr);
	return true;
}

bool
execute_command_with_specifiers_in_it(const struct wstring *wcmd_fmt, const struct format_arg *args)
{
	const wchar_t *wcmd_ptr = do_format(wcmd_fmt, args);
	struct wstring *wcmd = wcrtas(wcmd_ptr, wcslen(wcmd_ptr));
	if (wcmd == NULL) {
		return false;
	}
	struct string *cmd = convert_wstring_to_string(wcmd);
	free_wstring(wcmd);
	if (cmd == NULL) {
		return false;
	}
	info_status("Executing %s", cmd->ptr);
	// https://stackoverflow.com/questions/18678943/ncurses-shell-escape-drops-parent-process-output
	reset_shell_mode();
	int status = system(cmd->ptr);
	fflush(stdout);
	reset_prog_mode();
	clear();
	refresh();
	if (status == 0) {
		status_clean();
	} else {
		fail_status("Failed to execute %s", cmd->ptr);
	}
	free_string(cmd);
	return true;
}
