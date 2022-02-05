#include <stdio.h>
#include "feedeater.h"

bool
verify_config_values(void)
{
	bool success = true;
	if (cfg.size_conversion_threshold < 1000U) {
		success = false;
		fprintf(stderr, "Size conversion threshold must be greater than or equal to 1000!\n");
	}
	return success;
}
