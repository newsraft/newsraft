#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

bool
load_default_binds(void)
{
	if (assign_action_to_key('j',       INPUT_SELECT_NEXT)      == false) { goto error; }
	if (assign_action_to_key(KEY_DOWN,  INPUT_SELECT_NEXT)      == false) { goto error; }
	if (assign_action_to_key(KEY_NPAGE, INPUT_SELECT_NEXT_PAGE) == false) { goto error; }
	if (assign_action_to_key('k',       INPUT_SELECT_PREV)      == false) { goto error; }
	if (assign_action_to_key(KEY_UP,    INPUT_SELECT_PREV)      == false) { goto error; }
	if (assign_action_to_key(KEY_PPAGE, INPUT_SELECT_PREV_PAGE) == false) { goto error; }
	if (assign_action_to_key('g',       INPUT_SELECT_FIRST)     == false) { goto error; }
	if (assign_action_to_key(KEY_HOME,  INPUT_SELECT_FIRST)     == false) { goto error; }
	if (assign_action_to_key('G',       INPUT_SELECT_LAST)      == false) { goto error; }
	if (assign_action_to_key(KEY_END,   INPUT_SELECT_LAST)      == false) { goto error; }
	if (assign_action_to_key('\n',      INPUT_ENTER)            == false) { goto error; }
	if (assign_action_to_key(KEY_ENTER, INPUT_ENTER)            == false) { goto error; }
	if (assign_action_to_key('r',       INPUT_RELOAD)           == false) { goto error; }
	if (assign_action_to_key('R',       INPUT_RELOAD_ALL)       == false) { goto error; }
	if (assign_action_to_key('c',       INPUT_MARK_READ)        == false) { goto error; }
	if (assign_action_to_key('C',       INPUT_MARK_READ_ALL)    == false) { goto error; }
	if (assign_action_to_key('v',       INPUT_MARK_UNREAD)      == false) { goto error; }
	if (assign_action_to_key('V',       INPUT_MARK_UNREAD_ALL)  == false) { goto error; }
	if (assign_action_to_key('q',       INPUT_QUIT_SOFT)        == false) { goto error; }
	if (assign_action_to_key('Q',       INPUT_QUIT_HARD)        == false) { goto error; }
	return true;
error:
	free_binds();
	return false;
}

bool
assign_default_values_to_empty_config_strings(void)
{
#define ADVTECS(A, B, C, D) if (A == NULL) {         \
                            	A = malloc(B * D);   \
                            	if (A == NULL) {     \
                            		return false;    \
                            	}                    \
                            	memcpy(A, C, B * D); \
                            }
	// ATTENTION! Fourth argument to ADVTECS is number of characters (including terminator), not string length.
	ADVTECS(cfg.menu_set_entry_format,  sizeof(wchar_t), L"%4.0u │ %t",              11)
	ADVTECS(cfg.menu_item_entry_format, sizeof(wchar_t), L" %u │ %t",                9)
	ADVTECS(cfg.contents_meta_data,     sizeof(char),    "feed,title,authors,published,updated,max-summary-content", 57)
	ADVTECS(cfg.contents_date_format,   sizeof(char),    "%a, %d %b %Y %H:%M:%S %z", 25)
#undef ADVTECS
	return true;
}
