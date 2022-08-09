#include <stdio.h>
#include <unistd.h>
#include "load_config/load_config.h"

void
set_sane_value_for_update_threads_count(size_t initial_value)
{
	long online_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	INFO("Sysconf value of _SC_NPROCESSORS_ONLN equals to %ld.", online_cpus);
	if (online_cpus <= 1) {
		set_cfg_uint(CFG_UPDATE_THREADS_COUNT, 1);
	} else if (initial_value == 0) {
		set_cfg_uint(CFG_UPDATE_THREADS_COUNT, 2);
	} else {
		set_cfg_uint(CFG_UPDATE_THREADS_COUNT, MIN(initial_value, (size_t)online_cpus));
	}
}
