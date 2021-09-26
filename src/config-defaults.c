#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

size_t config_max_items = 100; // 0 == inf
size_t config_init_parser_buf_size = 100000;

/* don't initialize char pointers with string literals because then they will be immutable */
#define DEFAULT_CONFIG_MENU_SET_ENTRY_FORMAT "%-3n %t"
char *config_menu_set_entry_format = NULL;
#define DEFAULT_CONFIG_MENU_ITEM_ENTRY_FORMAT " %n  %t"
char *config_menu_item_entry_format = NULL;
#define DEFAULT_CONFIG_CONTENTS_META_DATA "feed,title,authors,categories,date,url,comments"
char *config_contents_meta_data = NULL;
#define DEFAULT_CONFIG_CONTENTS_DATE_FORMAT "%a, %d %b %Y %H:%M:%S %z"
char *config_contents_date_format = NULL;

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

int
assign_default_values_to_empty_config_strings(void)
{
	int error = 0;
	if (config_menu_set_entry_format == NULL) {
		config_menu_set_entry_format = malloc(sizeof(char) * (strlen(DEFAULT_CONFIG_MENU_SET_ENTRY_FORMAT) + 1));
		if (config_menu_set_entry_format == NULL) {
			error = 1;
		} else {
			strcpy(config_menu_set_entry_format, DEFAULT_CONFIG_MENU_SET_ENTRY_FORMAT);
		}
	}
	if (config_menu_item_entry_format == NULL) {
		config_menu_item_entry_format = malloc(sizeof(char) * (strlen(DEFAULT_CONFIG_MENU_ITEM_ENTRY_FORMAT) + 1));
		if (config_menu_item_entry_format == NULL) {
			error = 1;
		} else {
			strcpy(config_menu_item_entry_format, DEFAULT_CONFIG_MENU_ITEM_ENTRY_FORMAT);
		}
	}
	if (config_contents_meta_data == NULL) {
		config_contents_meta_data = malloc(sizeof(char) * (strlen(DEFAULT_CONFIG_CONTENTS_META_DATA) + 1));
		if (config_contents_meta_data == NULL) {
			error = 1;
		} else {
			strcpy(config_contents_meta_data, DEFAULT_CONFIG_CONTENTS_META_DATA);
		}
	}
	if (config_contents_date_format == NULL) {
		config_contents_date_format = malloc(sizeof(char) * (strlen(DEFAULT_CONFIG_CONTENTS_DATE_FORMAT) + 1));
		if (config_contents_date_format == NULL) {
			error = 1;
		} else {
			strcpy(config_contents_date_format, DEFAULT_CONFIG_CONTENTS_DATE_FORMAT);
		}
	}
	if (error != 0) {
		fprintf(stderr, "not enough memory for assigning default values to empty config strings\n");
	}
	return error;
}
