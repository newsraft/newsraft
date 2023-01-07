#include <string.h>
#include "newsraft.h"

static int64_t local_offset_to_utc;

static int64_t
get_timezone_offset_relative_to_utc(const char *timezone_str)
{
	if ((*timezone_str == '+') || (*timezone_str == '-')) {
		if (strlen(timezone_str) < 4) {
			WARN("Numeric time offset is too short!");
			return 0;
		}
		int64_t hours = 0, minutes = 0;
		if (timezone_str[3] == ':') {
			sscanf(timezone_str + 1, "%2" SCNd64 ":%2" SCNd64, &hours, &minutes);
		} else {
			sscanf(timezone_str + 1, "%2" SCNd64 "%2" SCNd64, &hours, &minutes);
		}
		int64_t offset = hours * 3600 + minutes * 60;
		return *timezone_str == '-' ? (-offset) : offset;
	} else {
		// Note to the future.
		// Check only for timezones that are mentioned in RFC 822,
		// otherwise it will grow into a mess very fast.
		if (strcmp(timezone_str, "UT") == 0) {
			return 0;
		} else if (strcmp(timezone_str, "GMT") == 0) {
			return 0;
		} else if (strcmp(timezone_str, "EST") == 0) {
			return -18000; // -5 * 3600
		} else if (strcmp(timezone_str, "EDT") == 0) {
			return -14400; // -4 * 3600
		} else if (strcmp(timezone_str, "CST") == 0) {
			return -21600; // -6 * 3600
		} else if (strcmp(timezone_str, "CDT") == 0) {
			return -18000; // -5 * 3600
		} else if (strcmp(timezone_str, "MST") == 0) {
			return -25200; // -7 * 3600
		} else if (strcmp(timezone_str, "MDT") == 0) {
			return -21600; // -6 * 3600
		} else if (strcmp(timezone_str, "PST") == 0) {
			return -28800; // -8 * 3600
		} else if (strcmp(timezone_str, "PDT") == 0) {
			return -25200; // -7 * 3600
		} else if (strcmp(timezone_str, "Z") == 0) {
			return 0;
		} else if (strcmp(timezone_str, "A") == 0) {
			return -3600;  // -1 * 3600
		} else if (strcmp(timezone_str, "M") == 0) {
			return -43200; // -12 * 3600
		} else if (strcmp(timezone_str, "N") == 0) {
			return 3600;   // +1 * 3600
		} else if (strcmp(timezone_str, "Y") == 0) {
			return 43200;  // +12 * 3600
		}
	}
	return 0;
}

int64_t
parse_date_rfc822(const struct string *value)
{
	if (value->len > 22) {
		struct tm t = {0};
		char *timezone_str = strptime(value->ptr, "%a, %d %b %Y %H:%M:%S ", &t);
		if (timezone_str != NULL) {
			int64_t time = (int64_t)mktime(&t);
			if (time > 0) {
				time -= get_timezone_offset_relative_to_utc(timezone_str);
				return time > 0 ? time : 0;
			}
		}
	}
	return 0;
}

int64_t
parse_date_rfc3339(const char *src, size_t src_len)
{
	if (src_len > 17) {
		struct tm t = {0};
		char *timezone_str = strptime(src, "%Y-%m-%dT%H:%M:%S", &t);
		if (timezone_str != NULL) {
			int64_t time = (int64_t)mktime(&t);
			if (time > 0) {
				time -= get_timezone_offset_relative_to_utc(timezone_str);
				return time > 0 ? time : 0;
			}
		}
	}
	return 0;
}

struct string *
get_config_date_str(int64_t date, config_entry_id format_index)
{
	char date_ptr[1000];
	const struct string *format = get_cfg_string(format_index);
	int64_t local_time = date + local_offset_to_utc;
	size_t date_len = strftime(date_ptr, 1000, format->ptr, localtime((time_t *)&local_time));
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

bool
get_local_offset_relative_to_utc(void)
{
	int64_t epoch_seconds = (int64_t)time(NULL);
	if (epoch_seconds < 0) {
		fputs("Failed to get system time!\n", stderr);
		return false;
	}
	// We don't need to free result of gmtime! See ctime(3).
	struct tm *utc_time = gmtime((time_t *)&epoch_seconds);
	if (utc_time == NULL) {
		fputs("Failed to get UTC time structure!\n", stderr);
		return false;
	}
	int64_t utc_seconds = (int64_t)mktime(utc_time);
	if (utc_seconds < 0) {
		fputs("Failed to get epoch time expressed in UTC!\n", stderr);
		return false;
	}
	local_offset_to_utc = epoch_seconds - utc_seconds;
	INFO("Local time offset relative to UTC is %" PRId64 ".", local_offset_to_utc);
	return true;
}
