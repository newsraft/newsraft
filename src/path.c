#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"

static char *conf_dir_path;
static char *data_dir_path;

int
set_conf_dir_path(void)
{
	conf_dir_path = malloc(sizeof(char) * MAXPATH);
	if (conf_dir_path == NULL) {
		fprintf(stderr, "failed to allocate memory for path to config directory\n");
		return 1;
	}

	// TODO: make mkdir recursive?

	DIR *d;
	char *env_var = getenv("FEEDEATER_CONFIG");
	if (env_var != NULL) {
		strcpy(conf_dir_path, env_var);
		mkdir(conf_dir_path, 0777);
		d = opendir(conf_dir_path);
		if (d != NULL) {
			closedir(d);
			return 0;
		}
	}

	env_var = getenv("XDG_CONFIG_HOME");
	if (env_var != NULL) {
		strcpy(conf_dir_path, env_var);
		strcat(conf_dir_path, "/feedeater");
		mkdir(conf_dir_path, 0777);
		d = opendir(conf_dir_path);
		if (d != NULL) {
			closedir(d);
			return 0;
		}
	}

	env_var = getenv("HOME");
	if (env_var != NULL) {
		strcpy(conf_dir_path, env_var);
		strcat(conf_dir_path, "/.config/feedeater");
		mkdir(conf_dir_path, 0777);
		d = opendir(conf_dir_path);
		if (d != NULL) {
			closedir(d);
			return 0;
		}
		strcpy(conf_dir_path, env_var);
		strcat(conf_dir_path, "/.feedeater");
		mkdir(conf_dir_path, 0777);
		d = opendir(conf_dir_path);
		if (d != NULL) {
			closedir(d);
			return 0;
		}
	}

	fprintf(stderr, "failed to find config directory\n");
	free(conf_dir_path);
	return 1;
}

void
free_conf_dir_path(void)
{
	free(conf_dir_path);
}

char *
get_config_file_path(char *file_name)
{
	char *path = malloc(MAXPATH * sizeof(char));
	if (path == NULL) {
		fprintf(stderr, "failed to allocate memory for path to \"%s\" config file\n", file_name);
		return NULL;
	}

	strcpy(path, conf_dir_path);
	strcat(path, "/");
	strcat(path, file_name);

	if (access(path, R_OK) == 0) {
		return path;
	}

	free(path);
	return NULL;
}

int
set_data_dir_path(void)
{
	data_dir_path = malloc(sizeof(char) * MAXPATH);
	if (data_dir_path == NULL) {
		fprintf(stderr, "failed to allocate memory for path to data directory\n");
		return 1;
	}

	// TODO: make mkdir recursive?

	DIR *d;
	char *env_var = getenv("FEEDEATER_DATA");
	if (env_var != NULL) {
		strcpy(data_dir_path, env_var);
		mkdir(data_dir_path, 0777);
		d = opendir(data_dir_path);
		if (d != NULL) {
			closedir(d);
			return 0;
		}
	}

	env_var = getenv("XDG_DATA_HOME");
	if (env_var != NULL) {
		strcpy(data_dir_path, env_var);
		strcat(data_dir_path, "/feedeater");
		mkdir(data_dir_path, 0777);
		d = opendir(data_dir_path);
		if (d != NULL) {
			closedir(d);
			return 0;
		}
	}

	env_var = getenv("HOME");
	if (env_var != NULL) {
		strcpy(data_dir_path, env_var);
		strcat(data_dir_path, "/.local/share/feedeater");
		mkdir(data_dir_path, 0777);
		d = opendir(data_dir_path);
		if (d != NULL) {
			closedir(d);
			return 0;
		}
	}

	fprintf(stderr, "failed to find data directory\n");
	free(data_dir_path);
	return 1;
}

void
free_data_dir_path(void)
{
	free(data_dir_path);
}

char *
get_db_path(void)
{
	char *path = malloc(sizeof(char) * MAXPATH);
	if (path == NULL) {
		status_write("failed to allocate memory for database path\n");
		return NULL;
	}
	strcpy(path, data_dir_path);
	strcat(path, "/feedeater.sqlite");
	return path;
}
