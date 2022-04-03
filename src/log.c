#include <stdio.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <tidy.h>
#include "feedeater.h"

FILE *log_stream = NULL;

static inline void
log_feedeater_version(void)
{
	INFO("feedeater version: " FEEDEATER_VERSION);
}

static inline void
log_ncurses_version(void)
{
	INFO("ncurses version: %s", curses_version());
}

static inline void
log_sqlite_version(void)
{
	INFO("SQLite version: %s", sqlite3_libversion());
}

static inline void
log_curl_version(void)
{
	INFO("curl version: %s", curl_version());
}

static inline void
log_tidy_version(void)
{
	INFO("LibTidy version: %s (%s) for %s", tidyLibraryVersion(), tidyReleaseDate(), tidyPlatform());
}

static inline void
log_cjson_version(void)
{
	INFO("cJSON version: %s", cJSON_Version());
}

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
	log_feedeater_version();
	log_ncurses_version();
	log_sqlite_version();
	log_curl_version();
	log_tidy_version();
	log_cjson_version();
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
