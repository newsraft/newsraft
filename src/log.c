#include <stdio.h>
#include <curl/curl.h>
#include <expat.h>
#include <yajl/yajl_version.h>
#include "newsraft.h"

FILE *log_stream = NULL;

bool
log_init(const char *path)
{
	if (path == NULL) {
		fputs("Path to the log file is not set!\n", stderr);
		return false;
	}
	log_stream = fopen(path, "w");
	if (log_stream == NULL) {
		fprintf(stderr, "Failed to open \"%s\" for writing a log information!\n", path);
		return false;
	}
	INFO("OK... Here we go. Focus. Speed. I am speed.");
	INFO("newsraft version: " NEWSRAFT_VERSION);
	INFO("ncurses version: %s", curses_version());
	INFO("SQLite version: %s", sqlite3_libversion());
	INFO("curl version: %s", curl_version());
	INFO("expat version: %s", XML_ExpatVersion());
	INFO("yajl version: %d.%d.%d", YAJL_MAJOR, YAJL_MINOR, YAJL_MICRO);
	return true;
}

void
log_stop(void)
{
	if (log_stream != NULL) {
		INFO("It's just an empty cup.");
		fclose(log_stream);
	}
}
