#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "feedeater.h"

static char *config_dir_path = NULL;
static char *data_dir_path = NULL;

static char *
get_config_dir(void)
{
	char *path = malloc(MAXPATH * sizeof(char));
	if (path == NULL) {
		fprintf(stderr, "failed to allocate memory for path to config directory\n");
		return NULL;
	}

	DIR *d;
	char *env_var = getenv("FEEDEATER_CONFIG");
	if (env_var != NULL) {
		strcpy(path, env_var);
		d = opendir(path);
		if (d != NULL) {
			closedir(d);
			return path;
		}
	}

	env_var = getenv("XDG_CONFIG_HOME");
	if (env_var != NULL) {
		strcpy(path, env_var);
		strcat(path, "/feedeater");
		d = opendir(path);
		if (d != NULL) {
			closedir(d);
			return path;
		}
	}

	env_var = getenv("HOME");
	if (env_var != NULL) {
		strcpy(path, env_var);
		strcat(path, "/.config/feedeater");
		d = opendir(path);
		if (d != NULL) {
			closedir(d);
			return path;
		}
		strcpy(path, env_var);
		strcat(path, "/.feedeater");
		d = opendir(path);
		if (d != NULL) {
			closedir(d);
			return path;
		}
	}

	strcpy(path, "/etc/feedeater");
	d = opendir(path);
	if (d != NULL) {
		closedir(d);
		return path;
	}

	fprintf(stderr, "failed to find config directory\n");
	free(path);
	return NULL;
}

char *
get_config_file_path(char *file_name)
{
	if (config_dir_path == NULL) {
		config_dir_path = get_config_dir();
		if (config_dir_path == NULL) return NULL;
	}

	char *path = malloc(MAXPATH * sizeof(char));
	if (path == NULL) {
		fprintf(stderr, "failed to allocate memory for path to \"%s\" config file\n", file_name);
		return NULL;
	}

	strcpy(path, config_dir_path);
	strcat(path, "/");
	strcat(path, file_name);

	if (access(path, R_OK) == 0) {
		return path;
	}

	free(path);
	return NULL;
}

static char *
get_data_dir_path(void)
{
	char *path = malloc(MAXPATH * sizeof(char));
	if (path == NULL) {
		fprintf(stderr, "failed to allocate memory for path to data directory\n");
		return NULL;
	}

	DIR *d;
	char *env_var = getenv("FEEDEATER_DATA");
	if (env_var != NULL) {
		strcpy(path, env_var);
		strcat(path, "/");
		mkdir(path, 0777);
		d = opendir(path);
		if (d != NULL) {
			closedir(d);
			return path;
		}
	}

	env_var = getenv("XDG_DATA_HOME");
	if (env_var != NULL) {
		strcpy(path, env_var);
		strcat(path, "/feedeater/");
		mkdir(path, 0777);
		d = opendir(path);
		if (d != NULL) {
			closedir(d);
			return path;
		}
	}

	env_var = getenv("HOME");
	if (env_var != NULL) {
		strcpy(path, env_var);
		strcat(path, "/.local/share/feedeater/");
		mkdir(path, 0777);
		d = opendir(path);
		if (d != NULL) {
			closedir(d);
			return path;
		}
	}

	fprintf(stderr, "failed to find data directory\n");
	free(path);
	return NULL;
}

char *
get_db_path(void)
{
	if (data_dir_path == NULL) {
		data_dir_path = get_data_dir_path();
		if (data_dir_path == NULL) return NULL;
	}
	char *path = malloc(sizeof(char) * MAXPATH);
	if (path == NULL) {
		status_write("failed to allocate memory for database path\n");
		return NULL;
	}
	strcpy(path, data_dir_path);
	strcat(path, "feedeater.sqlite");
	return path;
}
