#include <string.h>
#include <time.h>
#include <ctype.h>
#include "feedeater.h"

time_t
parse_date_rfc822(char *date_str, size_t date_len)
{
	if (date_len == 0) {
		return 0;
	}
	struct tm t = {0};
	if (isdigit(date_str[date_len - 1]) != 0) {
		if (strptime(date_str, "%a, %d %b %Y %H:%M:%S %z", &t) != NULL) {
			return mktime(&t);
		}
	} else {
		if (strptime(date_str, "%a, %d %b %Y %H:%M:%S %Z", &t) != NULL) {
			return mktime(&t);
		}
	}
	return 0;
}

time_t
parse_date_rfc3339(char *date_str, size_t date_len)
{
	if (date_len == 0) {
		return 0;
	}
	struct tm t = {0};
	if (isdigit(date_str[date_len - 1]) != 0) {
		if (strptime(date_str, "%Y-%m-%dT%H:%M:%S%z", &t) != NULL) {
			return mktime(&t);
		}
	} else {
		if (strptime(date_str, "%Y-%m-%dT%H:%M:%SZ", &t) != NULL) {
			return mktime(&t);
		}
	}
	return 0;
}

struct string *
get_config_date_str(time_t *time_ptr)
{
	struct tm ts = *gmtime(time_ptr);
	char time_str[200];
	if (strftime(time_str, sizeof(time_str), config_contents_date_format, &ts) != 0) {
		return create_string(time_str, strlen(time_str));
	}
	return NULL;
}
