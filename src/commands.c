#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

static inline void
execute_system_command(const char *cmd, bool quiet)
{
	info_status("Executing %s", cmd);
	if (!quiet) {
		pthread_mutex_lock(&interface_lock);
		NEWSRAFT_UI(ui_term());
	}
	int status = system(cmd);
	if (!quiet) {
		fflush(stdout);
		fflush(stderr);
		NEWSRAFT_UI(ui_init());
		NEWSRAFT_UI(ui_set_window_title());
		pthread_mutex_unlock(&interface_lock);
	}
	// Resizing could be handled by the program running on top, so we have to catch up.
	if (ui_is_running() && call_resize_handler_if_current_list_menu_size_is_different_from_actual() == false) {
		pthread_mutex_lock(&interface_lock);
		tb_clear();
		tb_present();
		status_recreate_unprotected();
		redraw_list_menu_unprotected();
		pthread_mutex_unlock(&interface_lock);
	}
	if (status == 0) {
		status_clean();
	} else {
		fail_status("Failed with status %d to run %s", status, cmd);
	}
}

void
copy_string_to_clipboard(const struct string *src)
{
	if (src != NULL && src->len > 0) {
		const struct string *cmd = get_cfg_string(NULL, CFG_COPY_TO_CLIPBOARD_COMMAND);
		if (strcmp(cmd->ptr, "newsraft-osc-52") == 0) {
			struct string *encoded_src = newsraft_base64_encode((uint8_t *)src->ptr, src->len);
			printf("\x1b]52;c;%s\x07", encoded_src->ptr);
			fflush(stdout);
			free_string(encoded_src);
		} else {
			FILE *p = popen(cmd->ptr, "w");
			if (p == NULL) {
				fail_status("Failed to execute clipboard command!");
				return;
			}
			fwrite(src->ptr, sizeof(char), src->len, p);
			pclose(p);
		}
		info_status("Copied %s", src->ptr);
	}
}

void
run_formatted_command(const struct wstring *wcmd_fmt, const struct format_arg *args, bool quiet)
{
	struct wstring *fmtout = wcrtes(200);
	do_format(fmtout, wcmd_fmt->ptr, args);
	struct string *cmd = convert_wstring_to_string(fmtout);
	if (cmd != NULL) {
		if (quiet) {
			struct string *quiet_cmd = crtes(cmd->len);
			str_appendf(quiet_cmd, "(%s) < /dev/null > /dev/null 2> /dev/null", cmd->ptr);
			execute_system_command(quiet_cmd->ptr, quiet);
			free_string(quiet_cmd);
		} else {
			execute_system_command(cmd->ptr, quiet);
		}
		free_string(cmd);
	}
	free_wstring(fmtout);
}
