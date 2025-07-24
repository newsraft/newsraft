#include <stdlib.h>
#include "newsraft.h"

static bool paint_it_black = true;
static volatile bool newsraft_has_successfully_initialized_ui = false;

pthread_mutex_t interface_lock = PTHREAD_MUTEX_INITIALIZER;

static bool
obtain_list_menu_size(size_t *width, size_t *height)
{
	int terminal_width = tb_width();
	// This is really critical! You will get integer overflow if terminal_width
	// is less than 12. You have been warned.
	if (terminal_width < 12) {
		FAIL("Terminal width of %d is too small!", terminal_width);
		return false;
	}
	int terminal_height = tb_height();
	if (terminal_height < 5) {
		FAIL("Terminal height of %d is too small!", terminal_height);
		return false;
	}

	INFO("Obtained terminal size: %d width, %d height.", terminal_width, terminal_height);

	*width = terminal_width;
	*height = terminal_height - 1; // Subtract 1 because we have status window.

	return true;
}

static int
ui_log_function(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int len = log_vprint("TERMBOX", fmt, args);
	va_end(args);
	return len;
}

bool
ui_init(void)
{
	tb_set_log_function(ui_log_function);
	int status = tb_init();
	if (status != TB_OK) {
		write_error("Initialization of user interface failed: %s.\n", tb_strerror(status));
		return false;
	}
	if (obtain_list_menu_size(&list_menu_width, &list_menu_height) == false) {
		write_error("Invalid terminal size obtained!\n");
		return false;
	}
	status = tb_hide_cursor();
	if (status != TB_OK) {
		WARN("Can't hide cursor: %s.", tb_strerror(status));
	}
	if (!get_cfg_bool(NULL, CFG_IGNORE_NO_COLOR) && getenv("NO_COLOR") != NULL) {
		INFO("NO_COLOR environment variable is set, canceling colors initialization.");
	} else {
		paint_it_black = false; // Some iridescent sensation at last!
	}
	newsraft_has_successfully_initialized_ui = true;
	return true;
}

void
ui_stop(void)
{
	tb_shutdown();
}

bool
ui_is_running(void)
{
	return newsraft_has_successfully_initialized_ui;
}

bool
run_menu_loop(void)
{
	struct timespec idling = {0, 100000000}; // 0.1 seconds
	struct menu_state *menu = setup_menu(&sections_menu_loop, NULL, NULL, 0, MENU_NORMAL, NULL);
	if (menu == NULL) {
		return false;
	}
	while (they_want_us_to_stop == false) {
		menu = menu->run(menu);
		if (menu == NULL) {
			break; // TODO: don't stop feed downloader?
			nanosleep(&idling, NULL); // Avoids CPU cycles waste while awaiting termination
			menu = setup_menu(&sections_menu_loop, NULL, NULL, 0, MENU_DISABLE_SETTINGS, NULL);
		}
	}
	return true;
}

input_id
resize_handler(void)
{
	pthread_mutex_lock(&interface_lock);

	// We need to call clear and refresh before all resize actions because
	// the junk text of previous size may remain in inactive areas.
	tb_clear();
	tb_present();

	if (obtain_list_menu_size(&list_menu_width, &list_menu_height) == false) {
		// Some really crazy resize happend. It is either a glitch or user
		// deliberately trying to break something. This state is unusable anyways.
		write_error("Don't flex around with me, okay?\n");
		goto error;
	}
	if (adjust_list_menu() == false) {
		goto error;
	}
	if (status_recreate_unprotected() == false) {
		goto error;
	}
	if (is_current_menu_a_pager() == true) {
		refresh_pager_menu();
	}
	redraw_list_menu_unprotected();
	pthread_mutex_unlock(&interface_lock);
	return INPUT_ERROR;
error:
	they_want_us_to_stop = true;
	pthread_mutex_unlock(&interface_lock);
	return INPUT_QUIT_HARD;
}

bool
call_resize_handler_if_current_list_menu_size_is_different_from_actual(void)
{
	size_t width = 0, height = 0;
	obtain_list_menu_size(&width, &height);
	if (width != list_menu_width || height != list_menu_height) {
		resize_handler();
		return true;
	}
	return false;
}

bool
arent_we_colorful(void)
{
	return !paint_it_black;
}
