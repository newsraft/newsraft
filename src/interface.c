#include <stdlib.h>
#include <stdio.h>
#include "load_config/load_config.h"
#include "newsraft.h"

static bool paint_it_black = true;
static volatile bool newsraft_has_successfully_initialized_ui = false;

bool hide_read_feeds;
bool hide_read_items;

pthread_mutex_t interface_lock = PTHREAD_MUTEX_INITIALIZER;

static bool
obtain_list_menu_size(size_t *width, size_t *height)
{
	int terminal_width = tb_width();
	if (terminal_width < 0) {
		FAIL("Terminal width of %d is invalid!", terminal_width);
		return false;
	}
	int terminal_height = tb_height();
	if (terminal_height < 0) {
		FAIL("Terminal height of %d is invalid!", terminal_height);
		return false;
	}

	INFO("Obtained terminal size: %d width, %d height.", terminal_width, terminal_height);

	*width = terminal_width;
	*height = terminal_height > 1 ? terminal_height - 1 : 0; // 1 row is reserved for status window

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
	INFO("Initializing user interface...");

	tb_set_log_function(ui_log_function);
	// Please, note that tb_init() hides the cursor automatically for us.
	int status = tb_init();
	if (status != TB_OK) {
		write_error("Initialization of user interface failed: %s.\n", tb_strerror(status));
		return false;
	}
	if (is_escape_key_used()) {
		WARN("Escape key has been bound - engaging ESC input mode!");
		tb_set_input_mode(TB_INPUT_ESC);
	} else {
		INFO("No Escape key binds was found - engaging ALT input mode.");
		tb_set_input_mode(TB_INPUT_ALT);
	}
	if (obtain_list_menu_size(&list_menu_width, &list_menu_height) == false) {
		write_error("Invalid terminal size obtained!\n");
		return false;
	}
	if (!get_cfg_bool(NULL, CFG_IGNORE_NO_COLOR) && getenv("NO_COLOR") != NULL) {
		INFO("NO_COLOR environment variable is set, canceling colors initialization.");
	} else {
		paint_it_black = false; // Some iridescent sensation at last!
		if (config_uses_256_colors) {
			tb_set_output_mode(TB_OUTPUT_256);
		}
	}
	adjust_list_menu();
	status_recreate_unprotected();
	enable_suspend_signal_handler();
	newsraft_has_successfully_initialized_ui = true;
	return true;
}

void
ui_term(void)
{
	INFO("Terminating user interface...");

	disable_suspend_signal_handler();
	status_delete_unprotected();
	free_list_menu();
	tb_shutdown();
}

void
ui_redraw_unprotected(void)
{
	tb_clear();
	tb_present();
	status_recreate_unprotected();
	redraw_list_menu_unprotected();
}

void
ui_suspend(void)
{
	if (!ui_is_running() || !is_suspend_signal_allowed()) {
		they_want_us_to_suspend = false;
		return;
	}
	INFO("Suspending user interface...");
	pthread_mutex_lock(&interface_lock);
	size_t prev_width = list_menu_width;
	size_t prev_height = list_menu_height;
	ui_term();
	fflush(stdout);
	fflush(stderr);

	// Here the whole process with all its threads gets stopped.
	suspend_process();
	// And here it continues whenever the user sends SIGCONT to it.

	if (they_want_us_to_stop) {
		pthread_mutex_unlock(&interface_lock);
		return;
	}

	INFO("Resuming user interface...");
	fflush(stdout);
	fflush(stderr);
	if (!ui_init()) {
		newsraft_has_successfully_initialized_ui = false;
		they_want_us_to_stop = true;
		pthread_mutex_unlock(&interface_lock);
		return;
	}
	ui_set_window_title();
	if (list_menu_width != prev_width || list_menu_height != prev_height) {
		pthread_mutex_unlock(&interface_lock);
		resize_handler();
	} else {
		ui_redraw_unprotected();
		pthread_mutex_unlock(&interface_lock);
	}
}

bool
ui_is_running(void)
{
	return newsraft_has_successfully_initialized_ui;
}

bool
ui_set_window_title(void)
{
	// setting the window title! escape codes are confusing
	fputs("\033]0;newsraft\007", stdout);
	fflush(stdout);
	return true;
}

bool
run_menu_loop(void)
{
	hide_read_feeds = get_cfg_bool(NULL, CFG_HIDE_READ_FEEDS);
	hide_read_items = get_cfg_bool(NULL, CFG_HIDE_READ_ITEMS);
	struct menu_state *menu = menu_setup(&sections_menu_loop, NULL, NULL, 0, MENU_NORMAL, NULL);
	while (menu && !they_want_us_to_stop) {
		menu = menu->run(menu); // TODO: don't stop feed downloader on NULL?
	}
	return true;
}

input_id
resize_handler(void)
{
	pthread_mutex_lock(&interface_lock);
	if (!obtain_list_menu_size(&list_menu_width, &list_menu_height)) {
		// Some really crazy resize happend. It is either a glitch or user
		// deliberately trying to break something. This state is unusable anyways.
		write_error("Don't flex around with me, okay?\n");
		they_want_us_to_stop = true;
		pthread_mutex_unlock(&interface_lock);
		return INPUT_QUIT_HARD;
	}
	adjust_list_menu();
	if (is_current_menu_a_pager()) {
		refresh_pager_menu();
	}
	ui_redraw_unprotected();
	pthread_mutex_unlock(&interface_lock);
	return INPUT_ERROR;
}

bool
arent_we_colorful(void)
{
	return !paint_it_black;
}
