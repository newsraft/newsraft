#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "newsraft.h"

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
parse_date_rfc3339(const char *src, size_t src_len)
{
	if (src_len == 0) {
		return 0;
	}
	struct tm t = {0};
	char *timezone_str = strptime(src, "%Y-%m-%dT%H:%M:%S", &t);
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

static inline time_t
get_local_time_offset_relative_to_utc(void)
{
	time_t rawtime = time(NULL);
	time_t offset = rawtime - mktime(gmtime(&rawtime));
	INFO("Local time offset relative to UTC: %ld", offset);
	return offset;
}

static inline struct string *
get_formatted_date_string(time_t time, const char *format)
{
	char date_ptr[1000];
	time_t local_time = time + get_local_time_offset_relative_to_utc();
	size_t date_len = strftime(date_ptr, 1000, format, localtime(&local_time));
	if (date_len == 0) {
		FAIL("Failed to create date string!");
		return NULL;
	}
	struct string *date_str = crtas(date_ptr, date_len);
	if (date_str == NULL) {
		FAIL("Not enough memory for date string!");
		return NULL;
	}
	return date_str;
}

struct string *
get_config_date_str(time_t date, enum config_entry_index format_index)
{
	const struct string *format = get_cfg_string(format_index);
	return get_formatted_date_string(date, format->ptr);
}

static inline void
http_response_date(char *buf, size_t buf_len, struct tm *tm)
{
	const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	snprintf(buf, buf_len, "%s, %d %s %d %02d:%02d:%02d GMT",
		days[tm->tm_wday], tm->tm_mday, months[tm->tm_mon],
		tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
}

static inline bool
http_response_date_now(char *buf, size_t buf_len, time_t date_value)
{
	time_t date = date_value;
	struct tm *tm = gmtime(&date);
	if (tm == NULL) {
		return false;
	}
	http_response_date(buf, buf_len, tm);
	return true;
}

struct string *
get_http_date_str(time_t date)
{
	char date_str[30];
	if (http_response_date_now(date_str, 30, date) == false) {
		return NULL;
	}
	return crtas(date_str, strlen(date_str));
}
