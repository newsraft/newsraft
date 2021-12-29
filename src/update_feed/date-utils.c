#include <time.h>
#include <ctype.h>
#include "feedeater.h"

static time_t
get_timezone_offset_relative_to_utc(const char *timezone_str)
{
	time_t offset = 0;
	if ((timezone_str[0] == '+') || (timezone_str[0] == '-')) {
		if (strlen(timezone_str) < 4) {
			WARN("Numeric time offset is too short!");
			return 0;
		}
		time_t hours, minutes;
		if (timezone_str[3] == ':') {
			sscanf(timezone_str + 1, "%2ld:%2ld", &hours, &minutes);
		} else {
			sscanf(timezone_str + 1, "%2ld%2ld", &hours, &minutes);
		}
		offset = hours * 3600 + minutes * 60;
		if (timezone_str[0] == '-') {
			offset = -offset;
		}
	} else {
		// (TODO) To add more abbreviations visit:
		// https://web.archive.org/web/20210320090620/https://en.wikipedia.org/wiki/List_of_time_zone_abbreviations
		if (strcmp(timezone_str, "EST") == 0) {
			offset = -18000; // -5 * 3600
		} else if (strcmp(timezone_str, "EDT") == 0) {
			offset = -14400; // -4 * 3600
		} else if (strcmp(timezone_str, "CST") == 0) {
			offset = -21600; // -6 * 3600
		} else if (strcmp(timezone_str, "CDT") == 0) {
			offset = -18000; // -5 * 3600
		} else if (strcmp(timezone_str, "MST") == 0) {
			offset = -25200; // -7 * 3600
		} else if (strcmp(timezone_str, "MDT") == 0) {
			offset = -21600; // -6 * 3600
		} else if (strcmp(timezone_str, "PST") == 0) {
			offset = -28800; // -8 * 3600
		} else if (strcmp(timezone_str, "PDT") == 0) {
			offset = -25200; // -7 * 3600
		} else if (strcmp(timezone_str, "A") == 0) {
			offset = -3600;  // -1 * 3600
		} else if (strcmp(timezone_str, "M") == 0) {
			offset = -43200; // -12 * 3600
		} else if (strcmp(timezone_str, "N") == 0) {
			offset = 3600;   // +1 * 3600
		} else if (strcmp(timezone_str, "Y") == 0) {
			offset = 43200;  // +12 * 3600
		//} else if (strcmp(timezone_str, "Z") == 0) {
			// offset = 0;
		//} else if (strcmp(timezone_str, "UT") == 0) {
			// offset = 0;
		//} else if (strcmp(timezone_str, "GMT") == 0) {
			// offset = 0;
		}
	}
	return offset;
}

// On success returns time of the date string.
// On failure returns 0.
time_t
parse_date_rfc822(const struct string *value)
{
	if (value->len == 0) {
		return 0;
	}
	struct tm t = {0};
	char *timezone_str = strptime(value->ptr, "%a, %d %b %Y %H:%M:%S ", &t);
	if (timezone_str == NULL) {
		return 0;
	}
	time_t time = mktime(&t);
	if (time == ((time_t) -1)) {
		return 0;
	}
	time -= get_timezone_offset_relative_to_utc(timezone_str);
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
	char *timezone_str = strptime(value->ptr, "%Y-%m-%dT%H:%M:%S", &t);
	if (timezone_str == NULL) {
		return 0;
	}
	time_t time = mktime(&t);
	if (time == ((time_t) -1)) {
		return 0;
	}
	time -= get_timezone_offset_relative_to_utc(timezone_str);
	return time;
}
