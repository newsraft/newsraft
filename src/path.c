#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"

#define CONF_DIR_PATH_ENVVAR "FEEDEATER_CONFIG_DIR"
#define DATA_DIR_PATH_ENVVAR "FEEDEATER_DATA_DIR"
#define FEEDS_FILE_PATH_ENVVAR "FEEDEATER_FEEDS_FILE"
#define DB_FILE_PATH_ENVVAR "FEEDEATER_DB_FILE"

static char *feeds_file_path = NULL;
static char *db_file_path = NULL;

static char *
get_conf_dir_path(void)
{
	char *conf_dir_path = malloc(sizeof(char) * MAXPATH);
	if (conf_dir_path == NULL) {
		fprintf(stderr, "failed to allocate memory for path to config directory\n");
		return NULL;
	}

	DIR *d;
	char *env_var = getenv(CONF_DIR_PATH_ENVVAR);
	if (env_var != NULL) {
		d = opendir(env_var);
		if (d != NULL) {
			closedir(d);
			strcpy(conf_dir_path, env_var);
			return conf_dir_path;
		}
		fprintf(stderr, CONF_DIR_PATH_ENVVAR " holds invalid path\n");
		free(conf_dir_path);
		return NULL;
	}

	env_var = getenv("XDG_CONFIG_HOME");
	if (env_var != NULL) {
		strcpy(conf_dir_path, env_var);
		mkdir(conf_dir_path, 0777);
		strcat(conf_dir_path, "/feedeater");
		mkdir(conf_dir_path, 0777);
		d = opendir(conf_dir_path);
		if (d != NULL) {
			closedir(d);
			return conf_dir_path;
		}
	}

	env_var = getenv("HOME");
	if (env_var != NULL) {
		strcpy(conf_dir_path, env_var);
		strcat(conf_dir_path, "/.config");
		mkdir(conf_dir_path, 0777);
		strcat(conf_dir_path, "/feedeater");
		mkdir(conf_dir_path, 0777);
		d = opendir(conf_dir_path);
		if (d != NULL) {
			closedir(d);
			return conf_dir_path;
		}
		strcpy(conf_dir_path, env_var);
		strcat(conf_dir_path, "/.feedeater");
		mkdir(conf_dir_path, 0777);
		d = opendir(conf_dir_path);
		if (d != NULL) {
			closedir(d);
			return conf_dir_path;
		}
	}

	fprintf(stderr, "failed to find config directory\n");
	free(conf_dir_path);
	return NULL;
}

static char *
get_data_dir_path(void)
{
	char *data_dir_path = malloc(sizeof(char) * MAXPATH);
	if (data_dir_path == NULL) {
		fprintf(stderr, "failed to allocate memory for path to data directory\n");
		return NULL;
	}

	DIR *d;
	char *env_var = getenv(DATA_DIR_PATH_ENVVAR);
	if (env_var != NULL) {
		d = opendir(env_var);
		if (d != NULL) {
			closedir(d);
			strcpy(data_dir_path, env_var);
			return data_dir_path;
		}
		fprintf(stderr, DATA_DIR_PATH_ENVVAR " holds invalid path\n");
		free(data_dir_path);
		return NULL;
	}

	env_var = getenv("XDG_DATA_HOME");
	if (env_var != NULL) {
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

	env_var = getenv("HOME");
	if (env_var != NULL) {
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

	fprintf(stderr, "failed to find data directory\n");
	free(data_dir_path);
	return NULL;
}

int
set_feeds_path(char *path)
{
	feeds_file_path = malloc(sizeof(char) * (strlen(path) + 1));
	if (feeds_file_path != NULL) {
		strcpy(feeds_file_path, path);
		return 0;
	}
	return 1;
}

int
set_db_path(char *path)
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
	char *feeds_file = getenv(FEEDS_FILE_PATH_ENVVAR);
	if (feeds_file != NULL) {
		strcpy(feeds_file_path, feeds_file);
		return feeds_file_path;
	}
	feeds_file = get_conf_dir_path();
	if (feeds_file == NULL) {
		return NULL;
	}
	strcat(feeds_file, "/feeds");
	return feeds_file;
}

char *
get_db_path(void)
{
	if (db_file_path != NULL) {
		return db_file_path;
	}
	char *db_file = getenv(DB_FILE_PATH_ENVVAR);
	if (db_file != NULL) {
		strcpy(db_file_path, db_file);
		return db_file_path;
	}
	db_file = get_data_dir_path();
	if (db_file == NULL) {
		return NULL;
	}
	strcat(db_file, "/feedeater.sqlite");
	return db_file;
}
