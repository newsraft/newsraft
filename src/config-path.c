#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "feedeater.h"

static char *config_dir_path = NULL;

static char *
get_config_dir(void)
{
	DIR *d;
	char *env_var, *path;

	path = malloc(MAXPATH * sizeof(char));
	if (path == NULL) {
		fprintf(stderr, "failed to allocate memory for path to config directory\n");
		return NULL;
	}

	env_var = getenv("FEEDEATER_HOME");
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

	fprintf(stderr, "failed to find config directory\n");
	free(path);
	return NULL;
}

char *
get_config_file_path(char *file_name)
{
	char *path;

	if (config_dir_path == NULL) {
		config_dir_path = get_config_dir();
		if (config_dir_path == NULL) return NULL;
	}

	path = malloc(MAXPATH * sizeof(char));
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
