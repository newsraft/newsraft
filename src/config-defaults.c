#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

size_t config_max_items = 100; // 0 == inf
size_t config_init_parser_buf_size = 100000;

// Don't initialize char pointers with string literals because then they will be immutable :(
char *config_menu_set_entry_format = NULL;
char *config_menu_item_entry_format = NULL;
char *config_contents_meta_data = NULL;
char *config_contents_date_format = NULL;
char *config_break_at = NULL;

int
load_default_binds(void)
{
	if (assign_command_to_key('j',       INPUT_SELECT_NEXT) != 0) return 1; // failure
	if (assign_command_to_key(KEY_DOWN,  INPUT_SELECT_NEXT) != 0) return 1; // failure
	if (assign_command_to_key(KEY_NPAGE, INPUT_SELECT_NEXT_PAGE) != 0) return 1; // failure
	if (assign_command_to_key('k',       INPUT_SELECT_PREV) != 0) return 1; // failure
	if (assign_command_to_key(KEY_UP,    INPUT_SELECT_PREV) != 0) return 1; // failure
	if (assign_command_to_key(KEY_PPAGE, INPUT_SELECT_PREV_PAGE) != 0) return 1; // failure
	if (assign_command_to_key('g',       INPUT_SELECT_FIRST) != 0) return 1; // failure
	if (assign_command_to_key(KEY_HOME,  INPUT_SELECT_FIRST) != 0) return 1; // failure
	if (assign_command_to_key('G',       INPUT_SELECT_LAST) != 0) return 1; // failure
	if (assign_command_to_key(KEY_END,   INPUT_SELECT_LAST) != 0) return 1; // failure
	if (assign_command_to_key('\n',      INPUT_ENTER) != 0) return 1; // failure
	if (assign_command_to_key(KEY_ENTER, INPUT_ENTER) != 0) return 1; // failure
	if (assign_command_to_key('r',       INPUT_RELOAD) != 0) return 1; // failure
	if (assign_command_to_key('R',       INPUT_RELOAD_ALL) != 0) return 1; // failure
	if (assign_command_to_key('c',       INPUT_MARK_READ) != 0) return 1; // failure
	if (assign_command_to_key('C',       INPUT_MARK_READ_ALL) != 0) return 1; // failure
	if (assign_command_to_key('v',       INPUT_MARK_UNREAD) != 0) return 1; // failure
	if (assign_command_to_key('V',       INPUT_MARK_UNREAD_ALL) != 0) return 1; // failure
	if (assign_command_to_key('q',       INPUT_QUIT_SOFT) != 0) return 1; // failure
	if (assign_command_to_key('Q',       INPUT_QUIT_HARD) != 0) return 1; // failure
	return 0; // success
}

static bool
assign_default_value_to_empty_config_string(char **str, const char *value, const size_t value_len)
{
	if (*str != NULL) {
		return true; // success, string already has some value
	}
	*str = malloc(sizeof(char) * (value_len + 1));
	if (*str == NULL) {
		return false; // failure
	}
	strncpy(*str, value, value_len);
	*(*str + value_len) = '\0';
	return true; // success
}

int
assign_default_values_to_empty_config_strings(void)
{
#define ADVTECS(A, B, C) if (assign_default_value_to_empty_config_string(A, B, C) == false) { return 1; /* failure */ }
	ADVTECS(&config_menu_set_entry_format, "%4.0u | %t", 10)
	ADVTECS(&config_menu_item_entry_format, " %u | %t", 8)
	ADVTECS(&config_contents_meta_data, "feed,title,authors,categories,date,url,comments,enclosures", 58)
	ADVTECS(&config_contents_date_format, "%a, %d %b %Y %H:%M:%S %z", 24)
	ADVTECS(&config_break_at, " \t!@*-+;:,./?", 13) // tab ('\t') is an individual character
	return 0; // success
}
