#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "feedeater.h"

struct string *
get_config_date_str(const time_t *time)
{
	struct tm ts = *gmtime(time);
	char time_ptr[200];
	if (strftime(time_ptr, sizeof(time_ptr), config_contents_date_format, &ts) == 0) {
		FAIL("Failed to create date string (strftime returned zero)!");
		return NULL; // failure
	}
	struct string *time_str = create_string(time_ptr, strlen(time_ptr));
	if (time_str == NULL) {
		FAIL("Not enough memory for date string creation (create_string returned NULL)!");
		return NULL; // failure
	}
	return time_str; // success
}

void
free_config_data(void)
{
	INFO("Freeing configuration strings.");
	free(config_menu_set_entry_format);
	free(config_menu_item_entry_format);
	free(config_contents_meta_data);
	free(config_contents_date_format);
	free(config_break_at);
}

int
load_config(void)
{
	int error = 0;
	/* if (parse_config_file() != 0) { */
	/* 	error = 1; */
	/* } */
	if (assign_default_values_to_empty_config_strings() != 0) {
		fprintf(stderr, "Not enough memory for assigning default values to empty config strings!\n");
		error = 1;
	}
	if (error != 0) {
		free_config_data();
	}
	return error;
}
