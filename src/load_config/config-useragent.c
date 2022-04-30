#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include "load_config/load_config.h"

struct string *
generate_useragent_string(void)
{
	struct string *ua = crtas("feedeater/", 10);
	if (ua == NULL) {
		goto error;
	}
	if (catas(ua, FEEDEATER_VERSION, strlen(FEEDEATER_VERSION)) == false) {
		goto error;
	}
	struct utsname sys_data;
	if (uname(&sys_data) == 0) {
		size_t sysname_len = strlen(sys_data.sysname);
		if (sysname_len != 0) {
			if (catas(ua, " (", 2) == false) {
				goto error;
			}
			if (catas(ua, sys_data.sysname, sysname_len) == false) {
				goto error;
			}
			if (catcs(ua, ')') == false) {
				goto error;
			}
		}
	}
	return ua;
error:
	fprintf(stderr, "Not enough memory for useragent string!\n");
	free_string(ua);
	return NULL;
}
