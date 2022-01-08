#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "feedeater.h"

struct config_data cfg;

static inline time_t
get_local_offset_relative_to_utc(void)
{
	time_t rawtime = time(NULL);
	return rawtime - mktime(gmtime(&rawtime));
}

struct string *
get_config_date_str(time_t time)
{
	char date_ptr[200];
	time_t local_time = time + get_local_offset_relative_to_utc();
	size_t date_len = strftime(date_ptr, sizeof(date_ptr), cfg.contents_date_format, localtime(&local_time));
	if (date_len == 0) {
		FAIL("Failed to create date string!");
		return NULL;
	}
	struct string *date_str = crtas(date_ptr, date_len);
	if (date_str == NULL) {
		FAIL("Not enough memory for date string!");
		return NULL;
	}
	return date_str;
}

void
free_config_data(void)
{
	INFO("Freeing configuration strings.");
	free(cfg.menu_set_entry_format);
	free(cfg.menu_item_entry_format);
	free(cfg.contents_meta_data);
	free(cfg.contents_date_format);
}

int
load_config(void)
{
	int error = 0;

	cfg.max_items = 0; // 0 == inf
	cfg.append_links = true;
	cfg.menu_set_entry_format = NULL;
	cfg.menu_item_entry_format = NULL;
	cfg.contents_meta_data = NULL;
	cfg.contents_date_format = NULL;

	/* if (parse_config_file() != 0) { */
	/* 	error = 1; */
	/* } */
	if (assign_default_values_to_empty_config_strings() == false) {
		fprintf(stderr, "Not enough memory for assigning default values to empty config strings!\n");
		error = 1;
	}
	if (error != 0) {
		free_config_data();
	}
	return error;
}
