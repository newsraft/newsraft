#include <string.h>
#include "feedeater.h"

static void
free_config_strings(void)
{
	free(config_menu_set_entry_format);
	free(config_menu_item_entry_format);
	free(config_contents_meta_data);
	free(config_contents_date_format);
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
	if (assign_default_values_to_empty_config_strings() != 0) {
		error = 1;
	}
	if (error != 0) {
		free_config_data();
	}
	return error;
}
