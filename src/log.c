#include <stdio.h>
#include "feedeater.h"

FILE *log_stream = NULL;

bool
log_init(const char *path)
{
	if (path == NULL) {
		fprintf(stderr, "Path to the log file is not set!\n");
		return false;
	}
	log_stream = fopen(path, "w");
	if (log_stream == NULL) {
		fprintf(stderr, "Failed to open \"%s\" for writing a log information!\n", path);
		return false;
	}
	INFO("Opened log file.");
	INFO("feedeater version: " FEEDEATER_VERSION);
	return true;
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
