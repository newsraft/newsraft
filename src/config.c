#include "feedeater.h"

struct config_data cfg;

void
free_config_data(void)
{
	INFO("Freeing configuration strings.");
	free_wstring(cfg.menu_section_entry_format);
	free_wstring(cfg.menu_feed_entry_format);
	free_wstring(cfg.menu_item_entry_format);
	free_string(cfg.proxy);
	free_string(cfg.proxy_auth);
	free_string(cfg.global_section_name);
	free_string(cfg.contents_meta_data);
	free_string(cfg.contents_date_format);
}

bool
load_config(void)
{
	if (assign_default_values_to_config_settings() == false) {
		return false;
	}

	/* if (parse_config_file() != 0) { */
	/* 	success = false; */
	/* } */

	if (verify_config_values() == false) {
		fprintf(stderr, "Verification of the configuration failed!\n");
		free_config_data();
		return false;
	}

	return true;
}
