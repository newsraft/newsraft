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
get_config_date_str(time_t date)
{
	return get_formatted_date_string(date, get_local_offset_relative_to_utc(), cfg.contents_date_format->ptr);
}
