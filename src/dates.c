#include "newsraft.h"

int64_t
parse_date_rfc3339(const char *src)
{
	struct tm t = {0};
	if (strptime(src, "%Y-%m-%dT%H:%M:%S", &t) != NULL) {
		int64_t time = (int64_t)mktime(&t);
		return time > 0 ? time : 0;
	}
	return 0;
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
