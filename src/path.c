#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include "feedeater.h"

// Note to the future.
// Do not read feedeater-specific file pathes from environment variables (like FEEDEATER_CONFIG_DIR),
// because environment is intended for settings that are valueable to many programs, not just a single one.

#define MAXPATH 4096 // maximum length of path

static char *feeds_file_path = NULL;
static char *config_file_path = NULL;
static char *db_file_path = NULL;

bool
set_feeds_path(const char *path)
{
	char *temp = realloc(feeds_file_path, sizeof(char) * (strlen(path) + 1));
	if (temp == NULL) {
		return false;
	}
	feeds_file_path = temp;
	strcpy(feeds_file_path, path);
	return true;
}

bool
set_config_path(const char *path)
{
	char *temp = realloc(config_file_path, sizeof(char) * (strlen(path) + 1));
	if (temp == NULL) {
		return false;
	}
	config_file_path = temp;
	strcpy(config_file_path, path);
	return true;
}

bool
set_db_path(const char *path)
{
	char *temp = realloc(db_file_path, sizeof(char) * (strlen(path) + 1));
	if (temp == NULL) {
		return false;
	}
	db_file_path = temp;
	strcpy(db_file_path, path);
	return true;
}

const char *
get_feeds_path(void)
{
	if (feeds_file_path != NULL) {
		return (const char *)feeds_file_path;
	}

	char *new_path = malloc(sizeof(char) * (MAXPATH + 1));
	if (new_path == NULL) {
		fprintf(stderr, "Not enough memory for feeds path!\n");
		return NULL;
	}

	FILE *f;
	char *env_var = getenv("XDG_CONFIG_HOME");
	size_t env_var_len;

	// Order in which to look up a feeds file:
	// 1. $XDG_CONFIG_HOME/feedeater/feeds
	// 2. $HOME/.config/feedeater/feeds
	// 3. $HOME/.feedeater/feeds

	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			strcpy(new_path, env_var);
			strcat(new_path, "/feedeater/feeds");
			f = fopen(new_path, "r");
			if (f != NULL) {
				fclose(f);
				feeds_file_path = new_path;
				return (const char *)feeds_file_path; // 1
			}
		}
	}

	env_var = getenv("HOME");
	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			strcpy(new_path, env_var);
			strcat(new_path, "/.config/feedeater/feeds");
			f = fopen(new_path, "r");
			if (f != NULL) {
				fclose(f);
				feeds_file_path = new_path;
				return (const char *)feeds_file_path; // 2
			}
			strcpy(new_path, env_var);
			strcat(new_path, "/.feedeater/feeds");
			f = fopen(new_path, "r");
			if (f != NULL) {
				fclose(f);
				feeds_file_path = new_path;
				return (const char *)feeds_file_path; // 3
			}
		}
	}

	fprintf(stderr, "Can not find feeds file!\n");

	free(new_path);

	return NULL;
}

const char *
get_config_path(void)
{
	if (config_file_path != NULL) {
		return (const char *)config_file_path;
	}

	char *new_path = malloc(sizeof(char) * (MAXPATH + 1));
	if (new_path == NULL) {
		fprintf(stderr, "Not enough memory for config path!\n");
		return NULL;
	}

	FILE *f;
	char *env_var = getenv("XDG_CONFIG_HOME");
	size_t env_var_len;

	// Order in which to look up a config file:
	// 1. $XDG_CONFIG_HOME/feedeater/config
	// 2. $HOME/.config/feedeater/config
	// 3. $HOME/.feedeater/config

	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			strcpy(new_path, env_var);
			strcat(new_path, "/feedeater/config");
			f = fopen(new_path, "r");
			if (f != NULL) {
				fclose(f);
				config_file_path = new_path;
				return (const char *)config_file_path; // 1
			}
		}
	}

	env_var = getenv("HOME");
	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			strcpy(new_path, env_var);
			strcat(new_path, "/.config/feedeater/config");
			f = fopen(new_path, "r");
			if (f != NULL) {
				fclose(f);
				config_file_path = new_path;
				return (const char *)config_file_path; // 2
			}
			strcpy(new_path, env_var);
			strcat(new_path, "/.feedeater/config");
			f = fopen(new_path, "r");
			if (f != NULL) {
				fclose(f);
				config_file_path = new_path;
				return (const char *)config_file_path; // 3
			}
		}
	}

	// Do not write error message because config file is optional.

	free(new_path);

	return NULL;
}

const char *
get_db_path(void)
{
	if (db_file_path != NULL) {
		return (const char *)db_file_path;
	}

	char *new_path = malloc(sizeof(char) * (MAXPATH + 1));
	if (new_path == NULL) {
		fprintf(stderr, "Not enough memory for database path!\n");
		return NULL;
	}

	FILE *f;
	char *env_var = getenv("XDG_DATA_HOME");
	size_t env_var_len;
	bool xdg_dir_is_set = false;
	bool home_dir_is_set = false;

	// Order in which to look up a database file:
	// 1. $XDG_DATA_HOME/feedeater/feedeater.sqlite3
	// 2. $HOME/.local/share/feedeater/feedeater.sqlite3

	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			xdg_dir_is_set = true;
			strcpy(new_path, env_var);
			strcat(new_path, "/feedeater/feedeater.sqlite3");
			f = fopen(new_path, "r");
			if (f != NULL) {
				fclose(f);
				db_file_path = new_path;
				return (const char *)db_file_path; // 1
			}
		}
	}

	env_var = getenv("HOME");
	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			home_dir_is_set = true;
			strcpy(new_path, env_var);
			strcat(new_path, "/.local/share/feedeater/feedeater.sqlite3");
			f = fopen(new_path, "r");
			if (f != NULL) {
				fclose(f);
				db_file_path = new_path;
				return (const char *)db_file_path; // 2
			}
		}
	}

	// If we got to this point then database file does not exist.
	// We have to create new one!

	DIR *d;

	if (xdg_dir_is_set == true) {
		env_var = getenv("XDG_DATA_HOME");
		strcpy(new_path, env_var);
		mkdir(new_path, 0777);
		strcat(new_path, "/feedeater");
		mkdir(new_path, 0777);
		d = opendir(new_path);
		if (d != NULL) {
			closedir(d);
			strcat(new_path, "/feedeater.sqlite3");
			db_file_path = new_path;
			return (const char *)db_file_path; // 1
		} else {
			fprintf(stderr, "Creation of \"%s\" directory has failed!\n", new_path);
		}
	} else if (home_dir_is_set == true) {
		env_var = getenv("HOME");
		strcpy(new_path, env_var);
		mkdir(new_path, 0777);
		strcat(new_path, "/.local");
		mkdir(new_path, 0777);
		strcat(new_path, "/share");
		mkdir(new_path, 0777);
		strcat(new_path, "/feedeater");
		mkdir(new_path, 0777);
		d = opendir(new_path);
		if (d != NULL) {
			closedir(d);
			strcat(new_path, "/feedeater.sqlite3");
			db_file_path = new_path;
			return (const char *)db_file_path; // 2
		} else {
			fprintf(stderr, "Creation of \"%s\" directory has failed!\n", new_path);
		}
	} else {
		fprintf(stderr, "Neither XDG_DATA_HOME or HOME is set!\n");
	}

	fprintf(stderr, "Creation of data directories failed!\n");

	free(new_path);

	return NULL;
}

void
free_feeds_path(void)
{
	free(feeds_file_path);
}

void
free_config_path(void)
{
	free(config_file_path);
}

void
free_db_path(void)
{
	free(db_file_path);
}
