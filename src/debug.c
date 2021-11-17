#include <stdio.h>
#include <stdarg.h>
#include "feedeater.h"

static FILE *f = NULL;

int
debug_init(const char *path)
{
	if (path == NULL) {
		fprintf(stderr, "path to the debug file is not set!\n");
		return 1; // failure
	}
	f = fopen(path, "w");
	if (f == NULL) {
		fprintf(stderr, "could not open \"%s\" for writing a debug information!\n", path);
		return 1; // failure
	}
	return 0; // success
}

void
debug_write(enum debug_level lvl, const char *format, ...)
{
	if (f == NULL) {
		return;
	}
	va_list args;
	va_start(args, format);
	switch (lvl) {
		case DBG_INFO: fprintf(f, "[INFO] "); break;
		case DBG_WARN: fprintf(f, "[WARN] "); break;
		case DBG_FAIL: fprintf(f, "[FAIL] "); break;
	}
	vfprintf(f, format, args);
	va_end(args);
}

void
debug_stop(void)
{
	if (f == NULL) {
		return;
	}
	fclose(f);
}
