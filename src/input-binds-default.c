#include "newsraft.h"

bool
assign_default_binds(void)
{
	if (assign_action_to_key("j",         1, INPUT_SELECT_NEXT)         == false) { goto error; }
	if (assign_action_to_key("KEY_DOWN",  8, INPUT_SELECT_NEXT)         == false) { goto error; }
	if (assign_action_to_key("KEY_NPAGE", 9, INPUT_SELECT_NEXT_PAGE)    == false) { goto error; }
	if (assign_action_to_key("k",         1, INPUT_SELECT_PREV)         == false) { goto error; }
	if (assign_action_to_key("KEY_UP",    6, INPUT_SELECT_PREV)         == false) { goto error; }
	if (assign_action_to_key("KEY_PPAGE", 9, INPUT_SELECT_PREV_PAGE)    == false) { goto error; }
	if (assign_action_to_key("g",         1, INPUT_SELECT_FIRST)        == false) { goto error; }
	if (assign_action_to_key("KEY_HOME",  8, INPUT_SELECT_FIRST)        == false) { goto error; }
	if (assign_action_to_key("G",         1, INPUT_SELECT_LAST)         == false) { goto error; }
	if (assign_action_to_key("KEY_END",   7, INPUT_SELECT_LAST)         == false) { goto error; }
	if (assign_action_to_key("^J",        2, INPUT_ENTER)               == false) { goto error; }
	if (assign_action_to_key("KEY_ENTER", 9, INPUT_ENTER)               == false) { goto error; }
	if (assign_action_to_key("r",         1, INPUT_RELOAD)              == false) { goto error; }
	if (assign_action_to_key("^R",        2, INPUT_RELOAD_ALL)          == false) { goto error; }
	if (assign_action_to_key("d",         1, INPUT_MARK_READ)           == false) { goto error; }
	if (assign_action_to_key("^D",        2, INPUT_MARK_READ_ALL)       == false) { goto error; }
	if (assign_action_to_key("D",         1, INPUT_MARK_UNREAD)         == false) { goto error; }
	if (assign_action_to_key("e",         1, INPUT_OVERVIEW_MENU)       == false) { goto error; }
	if (assign_action_to_key("p",         1, INPUT_STATUS_HISTORY_MENU) == false) { goto error; }
	if (assign_action_to_key("o",         1, INPUT_OPEN_IN_BROWSER)     == false) { goto error; }
	if (assign_action_to_key("c",         1, INPUT_COPY_TO_CLIPBOARD)   == false) { goto error; }
	if (assign_action_to_key("q",         1, INPUT_QUIT_SOFT)           == false) { goto error; }
	if (assign_action_to_key("Q",         1, INPUT_QUIT_HARD)           == false) { goto error; }
	return true;
error:
	fputs("Failed to assign default bindings!\n", stderr);
	free_binds();
	return false;
}
