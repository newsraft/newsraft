#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

bool
assign_default_values_to_config_settings(void)
{
	if ((cfg.menu_section_entry_format = wcrtas(L"%4.0u │ %t", 10)) == NULL) {
		goto undo0;
	}
	if ((cfg.menu_feed_entry_format = wcrtas(L"%4.0u │ %t", 10)) == NULL) {
		goto undo1;
	}
	if ((cfg.menu_item_entry_format = wcrtas(L" %u │ %t", 8)) == NULL) {
		goto undo2;
	}
	if ((cfg.proxy = crtes()) == NULL) {
		goto undo3;
	}
	if ((cfg.proxy_auth = crtes()) == NULL) {
		goto undo4;
	}
	if ((cfg.global_section_name = crtas("global", 6)) == NULL) {
		goto undo5;
	}
	if ((cfg.contents_meta_data = crtas("feed,title,authors,published,updated,max-summary-content", 56)) == NULL) {
		goto undo6;
	}
	if ((cfg.contents_date_format = crtas("%a, %d %b %Y %H:%M:%S %z", 24)) == NULL) {
		goto undo7;
	}
	cfg.max_items = 0; // 0 == inf
	cfg.append_links = true;
	cfg.run_cleaning_of_the_database_on_startup = true;
	cfg.run_analysis_of_the_database_on_startup = true;
	cfg.send_if_none_match_header = true;
	cfg.send_if_modified_since_header = true;
	cfg.size_conversion_threshold = 1200;
	return true;
undo7:
	free_string(cfg.contents_meta_data);
undo6:
	free_string(cfg.global_section_name);
undo5:
	free_string(cfg.proxy_auth);
undo4:
	free_string(cfg.proxy);
undo3:
	free_wstring(cfg.menu_item_entry_format);
undo2:
	free_wstring(cfg.menu_feed_entry_format);
undo1:
	free_wstring(cfg.menu_section_entry_format);
undo0:
	return false;
}

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
	if (assign_action_to_key('y',       INPUT_SECTIONS_MENU)    == false) { goto error; }
	if (assign_action_to_key('q',       INPUT_QUIT_SOFT)        == false) { goto error; }
	if (assign_action_to_key('Q',       INPUT_QUIT_HARD)        == false) { goto error; }
	return true;
error:
	free_binds();
	return false;
}
