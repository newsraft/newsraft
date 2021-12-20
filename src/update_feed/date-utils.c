#include <time.h>
#include <ctype.h>
#include "feedeater.h"

// On success returns time of the date string.
// On failure returns 0.
time_t
parse_date_rfc822(const struct string *value)
{
	if (value->len == 0) {
		return 0;
	}
	struct tm t = {0};
	time_t time = 0;
	if (isdigit(value->ptr[value->len - 1]) != 0) {
		if (strptime(value->ptr, "%a, %d %b %Y %H:%M:%S %z", &t) != NULL) {
			time = mktime(&t);
		}
	} else {
		if (strptime(value->ptr, "%a, %d %b %Y %H:%M:%S %Z", &t) != NULL) {
			time = mktime(&t);
		}
	}
	if (time == ((time_t) -1)) {
		return 0;
	}
	return time;
}

// On success returns time of the date string.
// On failure returns 0.
time_t
parse_date_rfc3339(const struct string *value)
{
	if (value->len == 0) {
		return 0;
	}
	struct tm t = {0};
	time_t time = 0;
	if (isdigit(value->ptr[value->len - 1]) != 0) {
		if (strptime(value->ptr, "%Y-%m-%dT%H:%M:%S%z", &t) != NULL) {
			time = mktime(&t);
		}
	} else {
		if (strptime(value->ptr, "%Y-%m-%dT%H:%M:%SZ", &t) != NULL) {
			time = mktime(&t);
		}
	}
	if (time == ((time_t) -1)) {
		return 0;
	}
	return time;
}
