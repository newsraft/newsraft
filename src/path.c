#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <openssl/sha.h>
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
	char *env_var = getenv("FEEDEATER_HOME");
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
		d = opendir(path);
		if (d != NULL) {
			closedir(d);
			return path;
		}
	}

	env_var = getenv("XDG_DATA_HOME");
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
		strcat(path, "/.local/share/feedeater");
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
get_data_dir_path_for_url(char *url)
{
	if (data_dir_path == NULL) {
		data_dir_path = get_data_dir();
		if (data_dir_path == NULL) return NULL;
	}

	char *path = malloc(MAXPATH * sizeof(char));
	if (path == NULL) {
		fprintf(stderr, "failed to allocate memory for path to \"%s\" data directory\n", url);
		return NULL;
	}

	SHA256_CTX c;
	unsigned char out[SHA256_DIGEST_LENGTH];
	char byte[3];

	SHA256_Init(&c);
	SHA256_Update(&c, url, strlen(url));
	SHA256_Final(out, &c);

	strcpy(path, data_dir_path);
	strcat(path, "/");
	for (int n = 0; n < SHA256_DIGEST_LENGTH; ++n) {
		sprintf(byte, "%02x", out[n]);
		strcat(path, byte);
	}

	return path;
}
