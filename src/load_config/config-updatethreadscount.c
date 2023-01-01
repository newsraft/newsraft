#include <unistd.h>
#include "load_config/load_config.h"

#ifdef __APPLE__
#define CPU_CORES_COUNT_COMMAND "sysctl -n hw.logicalcpu"
#else
#define CPU_CORES_COUNT_COMMAND "nproc"
#endif

void
set_sane_value_for_update_threads_count(size_t initial_value)
{
#ifdef _SC_NPROCESSORS_ONLN
	long online_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	INFO("Sysconf value of _SC_NPROCESSORS_ONLN equals to %ld.", online_cpus);
#else
	size_t online_cpus = 2;
	FILE *p = popen(CPU_CORES_COUNT_COMMAND, "r"); // Prints processors count.
	if (p != NULL) {
		if (fscanf(p, "%zu", &online_cpus) == 1) {
			INFO("Output of nproc command is %zu.", online_cpus);
		}
	}
#endif
	if (online_cpus <= 1) {
		set_cfg_uint(CFG_UPDATE_THREADS_COUNT, 1);
	} else if (initial_value == 0) {
		set_cfg_uint(CFG_UPDATE_THREADS_COUNT, (size_t)online_cpus);
	} else {
		set_cfg_uint(CFG_UPDATE_THREADS_COUNT, MIN(initial_value, (size_t)online_cpus));
	}
}
