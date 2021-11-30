#include <string.h>
#include <time.h>
#include <ctype.h>
#include "feedeater.h"

time_t
parse_date_rfc822(const char *date_str, size_t date_len)
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
parse_date_rfc3339(const char *date_str, size_t date_len)
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
get_config_date_str(const time_t *time)
{
	struct tm ts = *gmtime(time);
	char time_ptr[200];
	if (strftime(time_ptr, sizeof(time_ptr), config_contents_date_format, &ts) == 0) {
		debug_write(DBG_FAIL, "Failed to create date string (strftime returned zero)!\n");
		return NULL; // failure
	}
	struct string *time_str = create_string(time_ptr, strlen(time_ptr));
	if (time_str == NULL) {
		debug_write(DBG_FAIL, "Not enough memory for date string creation!\n");
		return NULL; // failure
	}
	return time_str; // success
}
