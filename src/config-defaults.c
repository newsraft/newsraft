#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

int
load_default_binds(void)
{
	if (assign_command_to_key('j',       INPUT_SELECT_NEXT)      != 0) { goto error; }
	if (assign_command_to_key(KEY_DOWN,  INPUT_SELECT_NEXT)      != 0) { goto error; }
	if (assign_command_to_key(KEY_NPAGE, INPUT_SELECT_NEXT_PAGE) != 0) { goto error; }
	if (assign_command_to_key('k',       INPUT_SELECT_PREV)      != 0) { goto error; }
	if (assign_command_to_key(KEY_UP,    INPUT_SELECT_PREV)      != 0) { goto error; }
	if (assign_command_to_key(KEY_PPAGE, INPUT_SELECT_PREV_PAGE) != 0) { goto error; }
	if (assign_command_to_key('g',       INPUT_SELECT_FIRST)     != 0) { goto error; }
	if (assign_command_to_key(KEY_HOME,  INPUT_SELECT_FIRST)     != 0) { goto error; }
	if (assign_command_to_key('G',       INPUT_SELECT_LAST)      != 0) { goto error; }
	if (assign_command_to_key(KEY_END,   INPUT_SELECT_LAST)      != 0) { goto error; }
	if (assign_command_to_key('\n',      INPUT_ENTER)            != 0) { goto error; }
	if (assign_command_to_key(KEY_ENTER, INPUT_ENTER)            != 0) { goto error; }
	if (assign_command_to_key('r',       INPUT_RELOAD)           != 0) { goto error; }
	if (assign_command_to_key('R',       INPUT_RELOAD_ALL)       != 0) { goto error; }
	if (assign_command_to_key('c',       INPUT_MARK_READ)        != 0) { goto error; }
	if (assign_command_to_key('C',       INPUT_MARK_READ_ALL)    != 0) { goto error; }
	if (assign_command_to_key('v',       INPUT_MARK_UNREAD)      != 0) { goto error; }
	if (assign_command_to_key('V',       INPUT_MARK_UNREAD_ALL)  != 0) { goto error; }
	if (assign_command_to_key('q',       INPUT_QUIT_SOFT)        != 0) { goto error; }
	if (assign_command_to_key('Q',       INPUT_QUIT_HARD)        != 0) { goto error; }
	return 0;
error:
	free_binds();
	return 1;
}

int
assign_default_values_to_empty_config_strings(void)
{
#define ADVTECS(A, B, C) if (A == NULL) {                 \
                         	A = malloc(sizeof(char) * C); \
                         	if (A == NULL) {              \
                         		return 1;                 \
                         	}                             \
                         	memcpy(A, B, C);              \
                         }
	// WARNING! Third argument to ADVTECS is number of bytes, not length.
	ADVTECS(cfg.menu_set_entry_format,  "%4.0u │ %t",               13)
	ADVTECS(cfg.menu_item_entry_format, " %u │ %t",                 11)
	ADVTECS(cfg.contents_meta_data,     "feed,title,authors,published,updated,max-summary-content", 57)
	ADVTECS(cfg.contents_date_format,   "%a, %d %b %Y %H:%M:%S %z", 25)
	ADVTECS(cfg.break_at,               " ",                        2)
#undef ADVTECS
	return 0;
}
