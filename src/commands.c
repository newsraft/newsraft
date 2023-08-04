#include <stdlib.h>
#include "newsraft.h"

static inline void
execute_system_command(const char *cmd)
{
	// https://stackoverflow.com/questions/18678943/ncurses-shell-escape-drops-parent-process-output
	info_status("Executing %s", cmd);
	pthread_mutex_lock(&interface_lock);
	curs_set(1); // Some programs expect that the cursor is enabled.
	reset_shell_mode();
	int status = system(cmd);
	fflush(stdout);
	reset_prog_mode();
	curs_set(0);
	pthread_mutex_unlock(&interface_lock);
	// Resizing could be handled by the program running on top, so we have to catch up.
	if (call_resize_handler_if_current_list_menu_size_is_different_from_actual() == false) {
		pthread_mutex_lock(&interface_lock);
		clear();
		refresh();
		status_recreate_unprotected();
		counter_recreate_unprotected();
		redraw_list_menu_unprotected();
		pthread_mutex_unlock(&interface_lock);
	}
	if (status == 0) {
		status_clean();
	} else {
		fail_status("Failed to execute %s", cmd);
	}
}

void
copy_string_to_clipboard(const struct string *src)
{
	if ((src == NULL) || (src->len == 0)) {
		info_status("Text you want to copy is empty!");
		return;
	}
	const struct string *copy_cmd = get_cfg_string(CFG_COPY_TO_CLIPBOARD_COMMAND);
	FILE *p = popen(copy_cmd->ptr, "w");
	if (p == NULL) {
		fail_status("Failed to execute clipboard command!");
		return;
	}
	fwrite(src->ptr, sizeof(char), src->len, p);
	pclose(p);
	good_status("Copied %s", src->ptr);
}

void
run_command_with_specifiers(const struct wstring *wcmd_fmt, const struct format_arg *args)
{
	const struct wstring *wcmd = do_format(wcmd_fmt->ptr, args);
	struct string *cmd = convert_wstring_to_string(wcmd);
	if (cmd != NULL) {
		execute_system_command(cmd->ptr);
		free_string(cmd);
	}
}
