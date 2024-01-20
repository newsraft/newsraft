#include <stdlib.h>
#include "newsraft.h"

static int64_t
get_local_offset_relative_to_utc(void)
{
	time_t utc_seconds = time(NULL);
	if (utc_seconds == ((time_t)-1)) return 0;
	struct tm *utc_time = gmtime(&utc_seconds);
	if (utc_time == NULL) return 0;
	time_t local_seconds = mktime(utc_time);
	if (local_seconds == ((time_t)-1)) return 0;
	return utc_seconds - local_seconds;
}

int64_t
parse_date_rfc3339(const char *src)
{
	struct tm t = {0};
	const char *rem = strptime(src, "%Y-%m-%dT%H:%M:%S", &t);
	if (rem == NULL) {
		return 0;
	}
	// Function timegm does the conversion without bias from local timezone,
	// but it's not in the standard, unfortunately...
	//int64_t time = (int64_t)timegm(&t);
	int64_t time = (int64_t)mktime(&t) + get_local_offset_relative_to_utc();
	if ((rem[0] == '+' || rem[0] == '-')
		&& ISDIGIT(rem[1]) && ISDIGIT(rem[2]) && rem[3] == ':' && ISDIGIT(rem[4]) && ISDIGIT(rem[5]))
	{
		const long hours = strtol(rem + 1, NULL, 10);
		const long minutes = strtol(rem + 4, NULL, 10);
		const long offset = hours * 3600 + minutes * 60;
		time = rem[0] == '+' ? time - offset : time + offset;
	}
	return time > 0 ? time : 0;
}

struct string *
get_config_date_str(int64_t date, config_entry_id format_index)
{
	const struct string *format = get_cfg_string(format_index);
	struct string *date_str = crtes(format->len + 1000);
	if (date_str == NULL) {
		FAIL("Not enough memory for date string!");
		return NULL;
	}
	struct tm localtime_data;
	size_t date_str_len = strftime(date_str->ptr, date_str->lim, format->ptr, localtime_r((time_t *)&date, &localtime_data));
	if (date_str_len == 0) {
		FAIL("Failed to create date string!");
		free_string(date_str);
		return NULL;
	}
	date_str->ptr[date_str_len] = '\0';
	date_str->len = date_str_len;
	return date_str;
}
