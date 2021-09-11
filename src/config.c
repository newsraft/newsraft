#include <string.h>
#include "feedeater.h"

#define DEFAULT_CONFIG_MENU_SET_ENTRY_FORMAT "%-3n %t"
#define DEFAULT_CONFIG_CONTENTS_META_DATA "feed,title,author,category,date,url,comments"
#define DEFAULT_CONFIG_CONTENTS_DATE_FORMAT "%a, %d %b %Y %H:%M:%S %z"

static void
free_config_strings(void)
{
	free(config_menu_set_entry_format);
	free(config_contents_meta_data);
	free(config_contents_date_format);
}

static int
set_config_default_strings(void)
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
		fprintf(stderr, "not enough memory for setting default values of config strings\n");
	}
	return error;
}

void
free_config_data(void)
{
	free_config_strings();
}

int
load_config(void)
{
	int error = 0;
	/* if (parse_config_file() != 0) { */
	/* 	error = 1; */
	/* } */
	if (set_config_default_strings() != 0) {
		error = 1;
	}
	if (error != 0) {
		free_config_data();
	}
	return error; // success
}
