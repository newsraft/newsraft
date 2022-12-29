#include <stdlib.h>
#include <curses.h>
#include "newsraft.h"

// Note to the future.
// These functions only return true if something actually happened (command was
// executed), because we need the boolean result of these functions as an
// indication whether the screen has to be redrawn or not.

static inline void
execute_system_command(const char *cmd)
{
	// https://stackoverflow.com/questions/18678943/ncurses-shell-escape-drops-parent-process-output
	info_status("Executing %s", cmd);
	pthread_mutex_lock(&interface_lock);
	reset_shell_mode();
	int status = system(cmd);
	fflush(stdout);
	reset_prog_mode();
	pthread_mutex_unlock(&interface_lock);
	// Resizing could be handled by the program running on top, so we have to catch up.
	resize_counter_action();
	if (status == 0) {
		status_clean();
	} else {
		fail_status("Failed to execute %s", cmd);
	}
}

bool
open_url_in_browser(const struct string *src)
{
	if ((src == NULL) || (src->len == 0)) {
		info_status("Link you want to open is empty!");
		return false;
	}
	const struct string *browser_cmd = get_cfg_string(CFG_OPEN_IN_BROWSER_COMMAND);
	struct string *cmd = crtss(browser_cmd);
	if (cmd != NULL) {
		if ((catcs(cmd, ' ') == true) && (catss(cmd, src) == true)) {
			execute_system_command(cmd->ptr);
			free_string(cmd);
			return true;
		}
		free_string(cmd);
	}
	return false;
}

bool
copy_string_to_clipboard(const struct string *src)
{
	if ((src == NULL) || (src->len == 0)) {
		info_status("Text you want to copy is empty!");
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
	struct string *cmd = convert_warray_to_string(wcmd_ptr, wcslen(wcmd_ptr));
	if (cmd == NULL) {
		return false;
	}
	execute_system_command(cmd->ptr);
	free_string(cmd);
	return true;
}
