#include "load_config/load_config.h"

static size_t
get_number_of_processors(void)
{
	const char *cmds[] = {
		"nproc 2>/dev/null",
		"sysctl -n hw.ncpu 2>/dev/null",
		"getconf _NPROCESSORS_ONLN 2>/dev/null",
		"grep -c ^processor /proc/cpuinfo 2>/dev/null",
		NULL
	};
	size_t cores = 0;
	for (const char **i = cmds; i != NULL; ++i) {
		FILE *p = popen(*i, "r");
		if (p != NULL) {
			fscanf(p, "%zu", &cores);
			INFO("Number of processors from %s is %zu", *i, cores);
			pclose(p);
			if (cores > 0) {
				return MIN(cores, 32); // Don't allow more than 32 threads!
			}
		}
	}
	return 1;
}

bool
load_config(void)
{
	const size_t threads_count = get_cfg_uint(CFG_UPDATE_THREADS_COUNT);
	const size_t cores_count = get_number_of_processors();
	set_cfg_uint(CFG_UPDATE_THREADS_COUNT, threads_count == 0 ? cores_count : MIN(threads_count, cores_count));

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

	return true;
}
