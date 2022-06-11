#include "load_config/load_config.h"

bool
load_config(void)
{
	/* if (parse_config_file() != 0) { */
	/* 	success = false; */
	/* } */

	if (assign_default_values_to_null_config_strings() == false) {
		fprintf(stderr, "Failed to assign default values to NULL config strings!\n");
		free_config();
		return false;
	}

	log_config_settings();

	if (verify_config_values() == false) {
		fprintf(stderr, "Verification of the configuration failed!\n");
		free_config();
		return false;
	}

	return true;
}
