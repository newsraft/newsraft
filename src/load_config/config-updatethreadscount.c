#include <stdio.h>
#include "load_config/load_config.h"

void
set_sane_value_for_update_threads_count(size_t initial_value)
{
	FILE *p = popen("nproc", "r"); // prints CPUs count
	if (p == NULL) {
		set_cfg_uint(CFG_UPDATE_THREADS_COUNT, 1);
	} else {
		size_t nproc_output;
		if (fscanf(p, "%zu", &nproc_output) != 1) {
			set_cfg_uint(CFG_UPDATE_THREADS_COUNT, 1);
		} else {
			if (initial_value == 0) {
				set_cfg_uint(CFG_UPDATE_THREADS_COUNT, nproc_output > 1 ? 2 : 1);
			} else {
				set_cfg_uint(CFG_UPDATE_THREADS_COUNT, MIN(initial_value, nproc_output));
			}
		}
		pclose(p);
	}
}
