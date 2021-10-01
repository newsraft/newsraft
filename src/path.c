#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"

// Note to the future.
// Do not read feedeater-specific file pathes from environment variables (like FEEDEATER_CONFIG_DIR),
// because environment is intended for settings that are valueable to many programs, not just a single one.

#define MAXPATH 4096 // maximum length of path

#define FEEDS_FILE_NAME "feeds"
#define CONFIG_FILE_NAME "config"
#define DB_FILE_NAME "feedeater.sqlite3"

static char *feeds_file_path = NULL;
static char *config_file_path = NULL;
static char *db_file_path = NULL;

static char *
get_conf_dir_path(void)
{
	char *conf_dir_path = malloc(sizeof(char) * (MAXPATH + 1));
	if (conf_dir_path == NULL) {
		fprintf(stderr, "failed to allocate memory for path to config directory\n");
		return NULL;
	}

	DIR *d;
	size_t env_var_len;

	char *env_var = getenv("XDG_CONFIG_HOME");
	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			strcpy(conf_dir_path, env_var);
			strcat(conf_dir_path, "/feedeater");
			d = opendir(conf_dir_path);
			if (d != NULL) {
				closedir(d);
				return conf_dir_path;
			}
		}
	}

	env_var = getenv("HOME");
	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			strcpy(conf_dir_path, env_var);
			strcat(conf_dir_path, "/.config/feedeater");
			d = opendir(conf_dir_path);
			if (d != NULL) {
				closedir(d);
				return conf_dir_path;
			}
			strcpy(conf_dir_path, env_var);
			strcat(conf_dir_path, "/.feedeater");
			d = opendir(conf_dir_path);
			if (d != NULL) {
				closedir(d);
				return conf_dir_path;
			}
		}
	}

	fprintf(stderr, "failed to find config directory\n");
	free(conf_dir_path);
	return NULL;
}

static char *
get_data_dir_path(void)
{
	char *data_dir_path = malloc(sizeof(char) * (MAXPATH + 1));
	if (data_dir_path == NULL) {
		fprintf(stderr, "failed to allocate memory for path to data directory\n");
		return NULL;
	}

	DIR *d;
	size_t env_var_len;

	char *env_var = getenv("XDG_DATA_HOME");
	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			strcpy(data_dir_path, env_var);
			mkdir(data_dir_path, 0777);
			strcat(data_dir_path, "/feedeater");
			mkdir(data_dir_path, 0777);
			d = opendir(data_dir_path);
			if (d != NULL) {
				closedir(d);
				return data_dir_path;
			}
		}
	}

	env_var = getenv("HOME");
	if (env_var != NULL) {
		env_var_len = strlen(env_var);
		if (env_var_len != 0) {
			strcpy(data_dir_path, env_var);
			strcat(data_dir_path, "/.local");
			mkdir(data_dir_path, 0777);
			strcat(data_dir_path, "/share");
			mkdir(data_dir_path, 0777);
			strcat(data_dir_path, "/feedeater");
			mkdir(data_dir_path, 0777);
			d = opendir(data_dir_path);
			if (d != NULL) {
				closedir(d);
				return data_dir_path;
			}
		}
	}

	fprintf(stderr, "failed to find data directory\n");
	free(data_dir_path);
	return NULL;
}

int
set_feeds_path(const char *path)
{
	feeds_file_path = malloc(sizeof(char) * (strlen(path) + 1));
	if (feeds_file_path != NULL) {
		strcpy(feeds_file_path, path);
		return 0;
	}
	return 1;
}

int
set_config_path(const char *path)
{
	config_file_path = malloc(sizeof(char) * (strlen(path) + 1));
	if (config_file_path != NULL) {
		strcpy(config_file_path, path);
		return 0;
	}
	return 1;
}

int
set_db_path(const char *path)
{
	db_file_path = malloc(sizeof(char) * (strlen(path) + 1));
	if (db_file_path != NULL) {
		strcpy(db_file_path, path);
		return 0;
	}
	return 1;
}

char *
get_feeds_path(void)
{
	if (feeds_file_path != NULL) {
		return feeds_file_path;
	}

	char *feeds_path = get_conf_dir_path();
	if (feeds_path != NULL) {
		strcat(feeds_path, "/" FEEDS_FILE_NAME);
		return feeds_path;
	}

	return NULL;
}

char *
get_config_path(void)
{
	if (config_file_path != NULL) {
		return config_file_path;
	}

	char *config_path = get_conf_dir_path();
	if (config_path != NULL) {
		strcat(config_path, "/" CONFIG_FILE_NAME);
		return config_path;
	}

	return NULL;
}

char *
get_db_path(void)
{
	if (db_file_path != NULL) {
		return db_file_path;
	}

	char *db_path = get_data_dir_path();
	if (db_path != NULL) {
		strcat(db_path, "/" DB_FILE_NAME);
		return db_path;
	}

	return NULL;
}
