#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "feedeater.h"

static inline time_t
get_local_offset_relative_to_utc(void)
{
	time_t rawtime = time(NULL);
	return rawtime - mktime(gmtime(&rawtime));
}

static inline struct string *
get_formatted_date_string(time_t time, time_t offset, const char *format)
{
	char date_ptr[333];
	time_t local_time = time + offset;
	size_t date_len = strftime(date_ptr, sizeof(date_ptr), format, localtime(&local_time));
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
	return get_formatted_date_string(date, get_local_offset_relative_to_utc(), format->ptr);
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
