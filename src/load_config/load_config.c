#include "load_config/load_config.h"

bool
load_config(void)
{
	const char *config_path = get_config_path();
	if (config_path != NULL) {
		if (parse_config_file(config_path) == false) {
			fputs("Failed to parse config file!\n", stderr);
			return false;
		}
	}

	if (assign_default_values_to_null_config_strings() == false) {
		fputs("Failed to assign default values to NULL config strings!\n", stderr);
		free_config();
		return false;
	}
	if (assign_calculated_values_to_auto_config_strings() == false) {
		fputs("Failed to assign calculated values to auto config strings!\n", stderr);
		free_config();
		return false;
	}

	log_config_settings();

	if (verify_config_values() == false) {
		fputs("Verification of the configuration failed!\n", stderr);
		free_config();
		return false;
	}

	return true;
}
