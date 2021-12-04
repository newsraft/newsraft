#include <stdio.h>
#include <stdarg.h>
#include "feedeater.h"

FILE *log_stream = NULL;

// On success returns 0.
// On failure returns non-zero.
int
log_init(const char *path)
{
	if (path == NULL) {
		fprintf(stderr, "Path to the log file is not set!\n");
		return 1;
	}
	log_stream = fopen(path, "w");
	if (log_stream == NULL) {
		fprintf(stderr, "Could not open \"%s\" for writing a log information!\n", path);
		return 1;
	}
	INFO("Opened log file.");
	INFO("feedeater version: " APPLICATION_VERSION);
	return 0;
}

void
log_stop(void)
{
	if (log_stream == NULL) {
		return;
	}
	INFO("Closing log file.");
	fclose(log_stream);
}
