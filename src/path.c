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
get_data_dir(void)
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
make_feed_dir(char *url)
{
	if (data_dir_path == NULL) {
		data_dir_path = get_data_dir();
		if (data_dir_path == NULL) return NULL;
	}

	char *file_name = malloc(MAXPATH * sizeof(char));
	if (file_name == NULL) {
		status_write("failed to allocate memory for file name string for \"%s\"\n", url);
		return NULL;
	}
	strcpy(file_name, url);

	if (strncmp(file_name, "https://", 8) == 0) {
		for (int i = 0; ; ++i) {
			file_name[i] = file_name[i + 8];
			if (file_name[i + 8] == '\0') break;
		}
	} else if (strncmp(file_name, "http://", 7) == 0) {
		for (int i = 0; ; ++i) {
			file_name[i] = file_name[i + 7];
			if (file_name[i + 7] == '\0') break;
		}
	}

	char *c;
	// replace all slashes with spaces
	while ((c = strchr(file_name, '/')) != NULL) *c = ' ';

	char *path = malloc(sizeof(char) * MAXPATH);
	if (path == NULL) {
		free(file_name);
		fprintf(stderr, "failed to allocate memory for path to \"%s\" data directory\n", url);
		return NULL;
	}

	strcpy(path, data_dir_path);
	strcat(path, file_name);
	strcat(path, "/");

	mkdir(path, 0777);

	char *elements_path = malloc(sizeof(char) * MAXPATH);
	if (elements_path == NULL) {
		free(file_name);
		fprintf(stderr, "failed to allocate memory for path to \"%s\" elements directory\n", url);
		return NULL;
	}

	strcpy(elements_path, path);
	strcat(elements_path, "elements");
	mkdir(elements_path, 0777);

	free(elements_path);
	free(file_name);

	return path;
}
