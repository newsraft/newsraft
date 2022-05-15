#include <stdio.h>
#include <string.h>
#include "load_config/load_config.h"

bool
verify_config_values(void)
{
	bool success = true;
	if (get_cfg_uint(CFG_SIZE_CONVERSION_THRESHOLD) < 1000U) {
		success = false;
		fprintf(stderr, "Size conversion threshold must be greater than or equal to 1000!\n");
	}
	const struct string *str = get_cfg_string(CFG_PROXY);
	if ((str->len != 0) && (strstr(str->ptr, "://") == NULL)) {
		success = false;
		fprintf(stderr, "The proxy string must be prefixed with scheme:// to specify which kind of proxy is used!\n");
	}
	return success;
}
