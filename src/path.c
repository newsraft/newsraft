#include <stdlib.h>
#include <dirent.h> // opendir
#include <string.h>
#include <limits.h> // PATH_MAX
#include <sys/stat.h> // mkdir
#include "newsraft.h"

// Note to the future.
// Do not read Newsraft-specific file pathes from environment variables (like
// NEWSRAFT_CONFIG_DIR), because environment is intended for settings that are
// valueable to many programs, not just a single one.

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static char feeds_file_path[PATH_MAX] = "";
static char config_file_path[PATH_MAX] = "";
static char db_file_path[PATH_MAX] = "";

static inline bool
set_file_path(char *dest, const char *name, const char *src)
{
	if (strlen(src) >= PATH_MAX) {
		write_error("Path to the %s file is too long!\n", name);
		return false;
	}
	strcpy(dest, src);
	return true;
}

bool
set_feeds_path(const char *path)
{
	return set_file_path(feeds_file_path, "feeds", path);
}

bool
set_config_path(const char *path)
{
	return set_file_path(config_file_path, "config", path);
}

bool
set_db_path(const char *path)
{
	return set_file_path(db_file_path, "database", path);
}

static inline bool
check_file_path(char *dest, const char *env_var, const char *file)
{
	const char *dir = getenv(env_var);
	if (!dir || strlen(dir) == 0) {
		return false;
	}
	snprintf(dest, PATH_MAX, "%s/%s", dir, file);
	FILE *f = fopen(dest, "r");
	if (!f) {
		return false;
	}
	fclose(f);
	return true;
}

const char *
get_feeds_path(void)
{
	// Order in which to look up a feeds file:
	// 1. $XDG_CONFIG_HOME/newsraft/feeds
	// 2. $HOME/.config/newsraft/feeds
	// 3. $HOME/.newsraft/feeds

	if (strlen(feeds_file_path) > 0
		|| check_file_path(feeds_file_path, "XDG_CONFIG_HOME", "newsraft/feeds")
		|| check_file_path(feeds_file_path, "HOME", ".config/newsraft/feeds")
		|| check_file_path(feeds_file_path, "HOME", ".newsraft/feeds"))
	{
		return feeds_file_path;
	}

	write_error("Can't find feeds file! Newsraft requires it to function.\n");
	write_error("A detailed description of the format of this file is provided in newsraft(1) man page.\n");
	return NULL;
}

const char *
get_config_path(void)
{
	// Order in which to look up a config file:
	// 1. $XDG_CONFIG_HOME/newsraft/config
	// 2. $HOME/.config/newsraft/config
	// 3. $HOME/.newsraft/config

	if (strlen(config_file_path) > 0
		|| check_file_path(config_file_path, "XDG_CONFIG_HOME", "newsraft/config")
		|| check_file_path(config_file_path, "HOME", ".config/newsraft/config")
		|| check_file_path(config_file_path, "HOME", ".newsraft/config"))
	{
		return config_file_path;
	}

	// Do not write error message because config file is optional.
	return NULL;
}

const char *
get_db_path(void)
{
	// Order in which to look up a database file:
	// 1. $XDG_DATA_HOME/newsraft/newsraft.sqlite3
	// 2. $HOME/.local/share/newsraft/newsraft.sqlite3
	// 3. $HOME/.newsraft/newsraft.sqlite3

	if (strlen(db_file_path) > 0
		|| check_file_path(db_file_path, "XDG_DATA_HOME", "newsraft/newsraft.sqlite3")
		|| check_file_path(db_file_path, "HOME", ".local/share/newsraft/newsraft.sqlite3")
		|| check_file_path(db_file_path, "HOME", ".newsraft/newsraft.sqlite3"))
	{
		return db_file_path;
	}

	// If we got to this point then database file does not exist.
	// We have to create a new one!

	char *xdg_data_home_var = getenv("XDG_DATA_HOME");
	char *home_var = getenv("HOME");

	if (xdg_data_home_var && strlen(xdg_data_home_var) > 0) {
		snprintf(db_file_path, PATH_MAX, "%s", xdg_data_home_var);
		mkdir(db_file_path, 0777);
		snprintf(db_file_path, PATH_MAX, "%s/newsraft", xdg_data_home_var);
		mkdir(db_file_path, 0777);
		DIR *d = opendir(db_file_path);
		if (d != NULL) {
			closedir(d);
			snprintf(db_file_path, PATH_MAX, "%s/newsraft/newsraft.sqlite3", xdg_data_home_var);
			return db_file_path; // 1
		} else {
			write_error("Failed to create \"%s\" directory!\n", db_file_path);
		}
	} else if (home_var && strlen(home_var) > 0) {
		snprintf(db_file_path, PATH_MAX, "%s", home_var);
		mkdir(db_file_path, 0777);
		snprintf(db_file_path, PATH_MAX, "%s/.local", home_var);
		mkdir(db_file_path, 0777);
		snprintf(db_file_path, PATH_MAX, "%s/.local/share", home_var);
		mkdir(db_file_path, 0777);
		snprintf(db_file_path, PATH_MAX, "%s/.local/share/newsraft", home_var);
		mkdir(db_file_path, 0777);
		DIR *d = opendir(db_file_path);
		if (d != NULL) {
			closedir(d);
			snprintf(db_file_path, PATH_MAX, "%s/.local/share/newsraft/newsraft.sqlite3", home_var);
			return db_file_path; // 2
		} else {
			write_error("Failed to create \"%s\" directory!\n", db_file_path);
		}
	} else {
		write_error("Neither XDG_DATA_HOME nor HOME environment variables are set!\n");
	}

	write_error("Failed to get database file path!\n");
	return NULL;
}
