#include "newsraft.h"

bool
assign_default_binds(void)
{
	if (assign_action_to_key('j',       INPUT_SELECT_NEXT)         == false) { goto error; }
	if (assign_action_to_key(KEY_DOWN,  INPUT_SELECT_NEXT)         == false) { goto error; }
	if (assign_action_to_key(KEY_NPAGE, INPUT_SELECT_NEXT_PAGE)    == false) { goto error; }
	if (assign_action_to_key('k',       INPUT_SELECT_PREV)         == false) { goto error; }
	if (assign_action_to_key(KEY_UP,    INPUT_SELECT_PREV)         == false) { goto error; }
	if (assign_action_to_key(KEY_PPAGE, INPUT_SELECT_PREV_PAGE)    == false) { goto error; }
	if (assign_action_to_key('g',       INPUT_SELECT_FIRST)        == false) { goto error; }
	if (assign_action_to_key(KEY_HOME,  INPUT_SELECT_FIRST)        == false) { goto error; }
	if (assign_action_to_key('G',       INPUT_SELECT_LAST)         == false) { goto error; }
	if (assign_action_to_key(KEY_END,   INPUT_SELECT_LAST)         == false) { goto error; }
	if (assign_action_to_key('\n',      INPUT_ENTER)               == false) { goto error; }
	if (assign_action_to_key(KEY_ENTER, INPUT_ENTER)               == false) { goto error; }
	if (assign_action_to_key('r',       INPUT_RELOAD)              == false) { goto error; }
	if (assign_action_to_key('R',       INPUT_RELOAD_ALL)          == false) { goto error; }
	if (assign_action_to_key('d',       INPUT_MARK_READ)           == false) { goto error; }
	if (assign_action_to_key('D',       INPUT_MARK_READ_ALL)       == false) { goto error; }
	if (assign_action_to_key('u',       INPUT_MARK_UNREAD)         == false) { goto error; }
	if (assign_action_to_key('U',       INPUT_MARK_UNREAD_ALL)     == false) { goto error; }
	if (assign_action_to_key('e',       INPUT_OVERVIEW_MENU)       == false) { goto error; }
	if (assign_action_to_key('y',       INPUT_SECTIONS_MENU)       == false) { goto error; }
	if (assign_action_to_key('p',       INPUT_STATUS_HISTORY_MENU) == false) { goto error; }
	if (assign_action_to_key('c',       INPUT_COPY_TO_CLIPBOARD)   == false) { goto error; }
	if (assign_action_to_key('q',       INPUT_QUIT_SOFT)           == false) { goto error; }
	if (assign_action_to_key('Q',       INPUT_QUIT_HARD)           == false) { goto error; }
	return true;
error:
	fputs("Failed to assign default bindings!\n", stderr);
	free_binds();
	return false;
}
