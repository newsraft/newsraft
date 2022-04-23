#include "feedeater.h"

struct config_data cfg;

void
free_config_data(void)
{
	INFO("Freeing configuration strings.");
	free_wstring(cfg.menu_section_entry_format);
	free_wstring(cfg.menu_feed_entry_format);
	free_wstring(cfg.menu_item_entry_format);
	free_string(cfg.global_section_name);
	free_string(cfg.contents_meta_data);
	free_string(cfg.contents_date_format);
	free_string(cfg.proxy);
	free_string(cfg.proxy_auth);
}

bool
load_config(void)
{
	bool success = true;

	cfg.max_items = 0; // 0 == inf
	cfg.append_links = true;
	cfg.run_cleaning_of_the_database_on_startup = true;
	cfg.run_analysis_of_the_database_on_startup = true;
	cfg.send_if_none_match_header = true;
	cfg.send_if_modified_since_header = true;
	cfg.menu_section_entry_format = NULL;
	cfg.menu_feed_entry_format = NULL;
	cfg.menu_item_entry_format = NULL;
	cfg.global_section_name = NULL;
	cfg.contents_meta_data = NULL;
	cfg.contents_date_format = NULL;
	cfg.proxy = NULL;
	cfg.proxy_auth = NULL;
	cfg.size_conversion_threshold = 1200;

	/* if (parse_config_file() != 0) { */
	/* 	success = false; */
	/* } */

	if (assign_default_values_to_empty_config_strings() == false) {
		fprintf(stderr, "Not enough memory for assigning default values to empty config strings!\n");
		success = false;
	}

	if ((success == true) && (verify_config_values() == false)) {
		fprintf(stderr, "Verification of the configuration failed!\n");
		success = false;
	}

	if (success == false) {
		free_config_data();
	}

	return success;
}
