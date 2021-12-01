#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

void
free_config_data(void)
{
	INFO("Freeing configuration strings.");
	free(config_menu_set_entry_format);
	free(config_menu_item_entry_format);
	free(config_contents_meta_data);
	free(config_contents_date_format);
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
		FAIL("Some error occurred during configuration loading!");
		free_config_data();
	}
	return error;
}
