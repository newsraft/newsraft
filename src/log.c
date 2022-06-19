#include <stdio.h>
#include <curl/curl.h>
#include <expat.h>
#include <yajl/yajl_version.h>
#include <tidy.h>
#include "newsraft.h"

FILE *log_stream = NULL;

static inline void
log_newsraft_version(void)
{
	INFO("newsraft version: " NEWSRAFT_VERSION);
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
log_expat_version(void)
{
	INFO("expat version: %s", XML_ExpatVersion());
}

static inline void
log_yajl_version(void)
{
	INFO("yajl version: %d", yajl_version());
}

static inline void
log_tidy_version(void)
{
	INFO("LibTidy version: %s (%s) for %s", tidyLibraryVersion(), tidyReleaseDate(), tidyPlatform());
}

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
	INFO("Opened log file.");
	log_newsraft_version();
	log_ncurses_version();
	log_sqlite_version();
	log_curl_version();
	log_expat_version();
	log_yajl_version();
	log_tidy_version();
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
