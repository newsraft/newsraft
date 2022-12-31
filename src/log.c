#include <curses.h>
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
		fputs("Failed to open log file for writing!\n", stderr);
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
log_stop(int error_code)
{
	if (log_stream != NULL) {
		INFO("Quitting the program with an exit code %d.", error_code);
		fclose(log_stream);
	}
}
