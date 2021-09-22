#include "feedeater.h"

/* the window user types to */
static WINDOW *input_win = NULL;

/* array of function pointers to input handlers */
static void (*input_handlers[INPUTS_COUNT])(void);

void
reset_input_handlers(void)
{
	for (int i = 0; i < INPUTS_COUNT; ++i) {
		input_handlers[i] = NULL;
	}
}

int
input_create(void)
{
	input_win = newwin(1, 1, LINES, COLS);
	if (input_win == NULL) {
		fprintf(stderr, "could not create input field\n");
		return 1;
	}

	if (cbreak() == ERR) {
		fprintf(stderr, "can't disable line buffering and erase/kill character-processing\n");
		delwin(input_win);
		return 1;
	}
	if (curs_set(0) == ERR) { // try to hide cursor
		debug_write(DBG_WARN, "can't hide cursor\n");
	}
	if (noecho() == ERR) {
		debug_write(DBG_WARN, "can't disable echoing of characters typed by the user\n");
	}
	if (keypad(input_win, TRUE) == ERR) { // used to enable arrow keys, function keys...
		debug_write(DBG_WARN, "can't enable extended keys\n");
	}

	return 0;
}

static int
get_input_command(void)
{
	int c = '\0';
	c = wgetch(input_win);
	if      (c == 'j'  || c == KEY_DOWN)   { return INPUT_SELECT_NEXT; }
	else if (c == 'k'  || c == KEY_UP)     { return INPUT_SELECT_PREV; }
	/*else if (ch == KEY_NPAGE)            { scroll_view(view_area_height); }*/
	/*else if (ch == KEY_PPAGE)            { scroll_view(-view_area_height); }*/
	else if (c == 'g'  || c == KEY_HOME)   { return INPUT_SELECT_FIRST; }
	else if (c == 'G'  || c == KEY_END)    { return INPUT_SELECT_LAST; }
	else if (c == '\n' || c == KEY_ENTER)  { return INPUT_ENTER; }
	else if (c == config_key_download)     { return INPUT_RELOAD; }
	else if (c == config_key_download_all) { return INPUT_RELOAD_ALL; }
	else if (c == config_key_soft_quit)    { return INPUT_SOFT_QUIT; }
	else if (c == config_key_hard_quit)    { return INPUT_HARD_QUIT; }
	else if (c == config_key_mark_read)    { return INPUT_MARK_READ; }
	else if (c == config_key_mark_unread)  { return INPUT_MARK_UNREAD; }
	else if (c == KEY_RESIZE) {
		/* rearrange list menu windows */
		resize_list_menu();

		/* recreate a status window */
		status_delete();
		status_create();
		status_clean();

		return INPUT_RESIZE;
	}
	return INPUTS_COUNT;
}

int
handle_input(void)
{
	int cmd;
	while (1) {
		cmd = get_input_command();
		if (cmd == INPUT_ENTER || cmd == INPUT_SOFT_QUIT || cmd == INPUT_HARD_QUIT) {
			return cmd;
		} else if (cmd == INPUTS_COUNT || input_handlers[cmd] == NULL) {
			continue;
		}
		input_handlers[cmd]();
	}
}

void
set_input_handler(enum input_cmd cmd, void (*func)(void))
{
	if (cmd < 0 || cmd >= INPUTS_COUNT) {
		return;
	}
	input_handlers[cmd] = func;
}

void
input_delete(void)
{
	delwin(input_win);
}
