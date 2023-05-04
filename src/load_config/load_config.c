#include "load_config/load_config.h"

static size_t
get_number_of_processors(void)
{
	const char *cmds[] = {
		"nproc 2>/dev/null",
		"sysctl -n hw.ncpu 2>/dev/null",
		"getconf _NPROCESSORS_ONLN 2>/dev/null",
		"grep -c ^processor /proc/cpuinfo 2>/dev/null",
		NULL,
	};
	size_t cpus = 0;
	for (const char **i = cmds; i != NULL; ++i) {
		FILE *p = popen(*i, "r");
		if (p != NULL) {
			fscanf(p, "%zu", &cpus);
			INFO("Number of processors from %s is %zu", *i, cpus);
			pclose(p);
		}
		if (cpus > 0) {
			return cpus;
		}
	}
	return 0;
}

bool
load_config(void)
{
	const size_t preferred_threads = get_cfg_uint(CFG_UPDATE_THREADS_COUNT);
	const size_t online_cpus = get_number_of_processors();
	const size_t real_threads = online_cpus > 1 ? online_cpus : 1;
	if (preferred_threads == 0) {
		set_cfg_uint(CFG_UPDATE_THREADS_COUNT, real_threads);
	} else {
		set_cfg_uint(CFG_UPDATE_THREADS_COUNT, MIN(preferred_threads, real_threads));
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

	return true;
}
