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
	for (const char **i = cmds; *i != NULL; ++i) {
		FILE *p = popen(*i, "r");
		if (p != NULL) {
			fscanf(p, "%zu", &cores);
			INFO("Number of processors from %s is %zu", *i, cores);
			pclose(p);
			if (cores > 0) {
				return cores;
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
	set_cfg_uint(CFG_UPDATE_THREADS_COUNT, MIN(threads_count > 0 ? threads_count : cores_count * 10, NEWSRAFT_THREADS_COUNT_LIMIT));
	if (prepare_config_string_settings() == false) {
		fputs("Failed to prepare config string settings!\n", stderr);
		free_config();
		return false;
	}
	log_config_settings();
	return true;
}
