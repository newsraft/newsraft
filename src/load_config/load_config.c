#include "load_config/load_config.h"

struct config_data cfg;

void
free_config(void)
{
	INFO("Freeing configuration strings.");
	free_wstring(cfg.menu_section_entry_format);
	free_wstring(cfg.menu_feed_entry_format);
	free_wstring(cfg.menu_item_entry_format);
	free_string(cfg.global_section_name);
	free_string(cfg.contents_meta_data);
	free_string(cfg.contents_date_format);
	free_string(cfg.useragent);
	free_string(cfg.proxy);
	free_string(cfg.proxy_auth);
	free_binds();
}

bool
load_config(void)
{
	if (assign_default_values_to_config_settings() == false) {
		fprintf(stderr, "Failed to assign default values to configuration settings!\n");
		return false;
	}
	if (load_default_binds() == false) {
		fprintf(stderr, "Failed to load default bindings!\n");
		free_config();
		return false;
	}

	/* if (parse_config_file() != 0) { */
	/* 	success = false; */
	/* } */

	log_config_settings();

	if (verify_config_values() == false) {
		fprintf(stderr, "Verification of the configuration failed!\n");
		free_config();
		return false;
	}

	return true;
}
