#include <stdio.h>
#include <string.h>
#include "feedeater.h"

bool
verify_config_values(void)
{
	bool success = true;
	if (cfg.size_conversion_threshold < 1000U) {
		success = false;
		fprintf(stderr, "Size conversion threshold must be greater than or equal to 1000!\n");
	}
	if ((cfg.proxy->len != 0) && (strstr(cfg.proxy->ptr, "://") == NULL)) {
		success = false;
		fprintf(stderr, "The proxy string must be prefixed with scheme:// to specify which kind of proxy is used!\n");
	}
	return success;
}
