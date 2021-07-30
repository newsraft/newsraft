#include <stdio.h>
#include <stdarg.h>
#include "feedeater.h"

static FILE *f = NULL;

int
debug_init(char *path)
{
	if (path == NULL) {
		fprintf(stderr, "path to debug file is empty!\n");
		return 1;
	}
	f = fopen(path, "w");
	if (f == NULL) {
		fprintf(stderr, "could not open \"%s\" for writing!\n", path);
		return 1;
	}
	return 0;
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
		case DBG_INFO: fprintf(f, "INFO    ) "); break;
		case DBG_OK:   fprintf(f, "OK      ] "); break;
		case DBG_WARN: fprintf(f, "WARNING } "); break;
		case DBG_ERR:  fprintf(f, "ERROR   > "); break;
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
