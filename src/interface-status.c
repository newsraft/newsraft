#include "feedeater.h"

// TODO respect status-messages-limit setting

static WINDOW *status_win;
static struct string_list *status_messages;
static size_t status_messages_count;
static size_t status_messages_limit;

bool
status_create(void)
{
	status_win = newwin(1, list_menu_width, list_menu_height, 0);
	if (status_win == NULL) {
		fprintf(stderr, "Failed to create status line window!\n");
		return false;
	}
	status_messages = NULL;
	status_messages_count = 0;
	status_messages_limit = get_cfg_uint(CFG_STATUS_MESSAGES_LIMIT);
	return true;
}

void
status_update(void)
{
	werase(status_win);
	if (status_messages != NULL) {
		mvwaddnstr(status_win, 0, 0, status_messages->str->ptr, list_menu_width);
	}
	wrefresh(status_win);
}

void
status_write(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	if ((status_messages == NULL) || (status_messages->str->len != 0)) {
		if (append_empty_string_to_string_list(&status_messages) == false) {
			return;
		}
	}
	if (string_vprintf(status_messages->str, format, args) == false) {
		va_end(args);
		return;
	}
	INFO("Last status message: %s", status_messages->str->ptr);
	status_update();
	va_end(args);
}

void
status_clean(void)
{
	werase(status_win);
	wrefresh(status_win);
	if (status_messages == NULL) {
		return;
	}
	if (status_messages->str->len == 0) {
		return;
	}
	append_empty_string_to_string_list(&status_messages);
}

void
status_resize(void)
{
	if (status_win != NULL) {
		delwin(status_win);
	}
	status_win = newwin(1, list_menu_width, list_menu_height, 0);
	if (status_win != NULL) {
		INFO("Created new status window.");
	} else {
		FAIL("Failed to create new status window!");
		return;
	}
	status_update();
}

void
status_delete(void)
{
	INFO("Freeing status window and status messages.");
	delwin(status_win);
	free_string_list(status_messages);
}
