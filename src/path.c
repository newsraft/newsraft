#include <stdio.h>
#include <stdlib.h>
#include <dirent.h> // opendir
#include <string.h>
#include <limits.h> // PATH_MAX
#include <sys/stat.h> // mkdir
#include "newsraft.h"

// Note to the future.
// Do not read Newsraft-specific file pathes from environment variables (like NEWSRAFT_CONFIG_DIR),
// because environment is intended for settings that are valueable to many programs, not just a single one.

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static char feeds_file_path[PATH_MAX] = "";
static char config_file_path[PATH_MAX] = "";
static char db_file_path[PATH_MAX] = "";

bool
set_feeds_path(const char *path)
{
	if (strlen(path) >= PATH_MAX) {
		fputs("Path to the feeds file is too long!\n", stderr);
		return false;
	}
	strcpy(feeds_file_path, path);
	return true;
}

bool
set_config_path(const char *path)
{
	if (strlen(path) >= PATH_MAX) {
		fputs("Path to the config file is too long!\n", stderr);
		return false;
	}
	strcpy(config_file_path, path);
	return true;
}

bool
set_db_path(const char *path)
{
	if (strlen(path) >= PATH_MAX) {
		fputs("Path to the database file is too long!\n", stderr);
		return false;
	}
	strcpy(db_file_path, path);
	return true;
}

const char *
get_feeds_path(void)
{
	if (strlen(feeds_file_path) != 0) {
		return feeds_file_path;
	}

	FILE *f;
	char *env_var = getenv("XDG_CONFIG_HOME");
	size_t env_var_len;

	// Order in which to look up a feeds file:
	// 1. $XDG_CONFIG_HOME/newsraft/feeds
	// 2. $HOME/.config/newsraft/feeds
	// 3. $HOME/.newsraft/feeds

	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			strcpy(feeds_file_path, env_var);
			strcat(feeds_file_path, "/newsraft/feeds");
			f = fopen(feeds_file_path, "r");
			if (f != NULL) {
				fclose(f);
				return feeds_file_path; // 1
			}
		}
	}

	env_var = getenv("HOME");
	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			strcpy(feeds_file_path, env_var);
			strcat(feeds_file_path, "/.config/newsraft/feeds");
			f = fopen(feeds_file_path, "r");
			if (f != NULL) {
				fclose(f);
				return feeds_file_path; // 2
			}
			strcpy(feeds_file_path, env_var);
			strcat(feeds_file_path, "/.newsraft/feeds");
			f = fopen(feeds_file_path, "r");
			if (f != NULL) {
				fclose(f);
				return feeds_file_path; // 3
			}
		}
	}

	fputs("Can't find feeds file!\n", stderr);
	return NULL;
}

const char *
get_config_path(void)
{
	if (strlen(config_file_path) != 0) {
		return config_file_path;
	}

	FILE *f;
	char *env_var = getenv("XDG_CONFIG_HOME");
	size_t env_var_len;

	// Order in which to look up a config file:
	// 1. $XDG_CONFIG_HOME/newsraft/config
	// 2. $HOME/.config/newsraft/config
	// 3. $HOME/.newsraft/config

	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			strcpy(config_file_path, env_var);
			strcat(config_file_path, "/newsraft/config");
			f = fopen(config_file_path, "r");
			if (f != NULL) {
				fclose(f);
				return config_file_path; // 1
			}
		}
	}

	env_var = getenv("HOME");
	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			strcpy(config_file_path, env_var);
			strcat(config_file_path, "/.config/newsraft/config");
			f = fopen(config_file_path, "r");
			if (f != NULL) {
				fclose(f);
				return config_file_path; // 2
			}
			strcpy(config_file_path, env_var);
			strcat(config_file_path, "/.newsraft/config");
			f = fopen(config_file_path, "r");
			if (f != NULL) {
				fclose(f);
				return config_file_path; // 3
			}
		}
	}

	// Do not write error message because config file is optional.
	return NULL;
}

const char *
get_db_path(void)
{
	if (strlen(db_file_path) != 0) {
		return db_file_path;
	}

	FILE *f;
	char *env_var = getenv("XDG_DATA_HOME");
	size_t env_var_len;
	bool xdg_dir_is_set = false;
	bool home_dir_is_set = false;

	// Order in which to look up a database file:
	// 1. $XDG_DATA_HOME/newsraft/newsraft.sqlite3
	// 2. $HOME/.local/share/newsraft/newsraft.sqlite3

	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			xdg_dir_is_set = true;
			strcpy(db_file_path, env_var);
			strcat(db_file_path, "/newsraft/newsraft.sqlite3");
			f = fopen(db_file_path, "r");
			if (f != NULL) {
				fclose(f);
				return db_file_path; // 1
			}
		}
	}

	env_var = getenv("HOME");
	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			home_dir_is_set = true;
			strcpy(db_file_path, env_var);
			strcat(db_file_path, "/.local/share/newsraft/newsraft.sqlite3");
			f = fopen(db_file_path, "r");
			if (f != NULL) {
				fclose(f);
				return db_file_path; // 2
			}
		}
	}

	// If we got to this point then database file does not exist.
	// We have to create new one!

	DIR *d;

	if (xdg_dir_is_set == true) {
		env_var = getenv("XDG_DATA_HOME");
		strcpy(db_file_path, env_var);
		mkdir(db_file_path, 0777);
		strcat(db_file_path, "/newsraft");
		mkdir(db_file_path, 0777);
		d = opendir(db_file_path);
		if (d != NULL) {
			closedir(d);
			strcat(db_file_path, "/newsraft.sqlite3");
			return db_file_path; // 1
		} else {
			fprintf(stderr, "Failed to create \"%s\" directory!\n", db_file_path);
		}
	} else if (home_dir_is_set == true) {
		env_var = getenv("HOME");
		strcpy(db_file_path, env_var);
		mkdir(db_file_path, 0777);
		strcat(db_file_path, "/.local");
		mkdir(db_file_path, 0777);
		strcat(db_file_path, "/share");
		mkdir(db_file_path, 0777);
		strcat(db_file_path, "/newsraft");
		mkdir(db_file_path, 0777);
		d = opendir(db_file_path);
		if (d != NULL) {
			closedir(d);
			strcat(db_file_path, "/newsraft.sqlite3");
			return db_file_path; // 2
		} else {
			fprintf(stderr, "Failed to create \"%s\" directory!\n", db_file_path);
		}
	} else {
		fputs("Neither XDG_DATA_HOME or HOME is set!\n", stderr);
	}

	fputs("Failed to get database file path!\n", stderr);
	return NULL;
}
