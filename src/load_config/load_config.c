#include <unistd.h>
#include "load_config/load_config.h"

bool
load_config(void)
{
	const size_t preferred_threads = get_cfg_uint(CFG_UPDATE_THREADS_COUNT);
	const long online_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	INFO("Sysconf value of _SC_NPROCESSORS_ONLN is %ld", online_cpus);
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
