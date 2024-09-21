#include <curl/curl.h>
#include <expat.h>
#include <yajl/yajl_version.h>
#include "newsraft.h"

static FILE *log_stream = NULL;

bool
log_init(const char *path)
{
	if (path == NULL) {
		write_error("Path to the log file is not set!\n");
		return false;
	}
	log_stream = fopen(path, "w");
	if (log_stream == NULL) {
		write_error("Failed to open log file for writing!\n");
		return false;
	}
	INFO("Okay... Here we go. Focus. Speed. I am speed.");
	INFO("newsraft version: %s", NEWSRAFT_VERSION);
	INFO("ncurses version: %s", curses_version());
	INFO("SQLite version: %s", sqlite3_libversion());
	INFO("curl version: %s", curl_version());
	INFO("expat version: %s", XML_ExpatVersion());
	INFO("yajl version: %d.%d.%d", YAJL_MAJOR, YAJL_MINOR, YAJL_MICRO);
	return true;
}

void
log_write(const char *prefix, const char *format, ...)
{
	if (log_stream != NULL) {
		struct timespec t = {0};
		clock_gettime(CLOCK_MONOTONIC_RAW, &t);
		va_list args;
		va_start(args, format);
		fprintf(log_stream, "%02d:%02d:%02d.%03d [%s] ",
			(int)(t.tv_sec / 3600),
			(int)(t.tv_sec / 60 % 60),
			(int)(t.tv_sec % 60),
			(int)(t.tv_nsec / 1000000),
			prefix
		);
		vfprintf(log_stream, format, args);
		fputc('\n', log_stream);
		fflush(log_stream);
		va_end(args);
	}
}

FILE *
log_get_stream(void)
{
	return log_stream;
}

void
log_stop(int error_code)
{
	if (log_stream != NULL) {
		INFO("Quitting the program with an exit code %d.", error_code);
		fclose(log_stream);
	}
}
