#include <stdlib.h>
#include <curl/curl.h>
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

static int64_t
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

int64_t
parse_date(const char *str, bool rfc3339_first)
{
	int64_t date = 0;

	if (rfc3339_first == true) {
		date = parse_date_rfc3339(str);
	}
	if (date <= 0) {
		date = (int64_t)curl_getdate(str, NULL);
		if (date < 0) {
			date = 0;
		}
	}
	if (date <= 0 && rfc3339_first == false) {
		date = parse_date_rfc3339(str);
	}

	static const char *formats[] = {
		"%Y-%m-%d",
		"%Y/%m/%d",
	};
	for (size_t i = 0; i < LENGTH(formats) && date <= 0; ++i) {
		struct tm t = {0};
		if (strptime(str, formats[i], &t)) {
			date = (int64_t)mktime(&t) + get_local_offset_relative_to_utc();
			if (date < 0) {
				date = 0;
			}
		}
	}

	return date;
}

struct string *
get_cfg_date(struct config_context **ctx, config_entry_id format_id, int64_t date)
{
	const struct string *format = get_cfg_string(ctx, format_id);
	struct string *str = crtes(format->len + 1000);
	if (str == NULL) {
		return NULL;
	}
	struct tm timedata;
	str->len = strftime(str->ptr, str->lim, format->ptr, localtime_r((time_t *)&date, &timedata));
	str->ptr[str->len] = '\0';
	if (str->len == 0) {
		WARN("Failed to format date string!");
	}
	return str;
}

struct timespec
newsraft_get_monotonic_time(void)
{
	struct timespec t = {};
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t;
}

struct string *
newsraft_get_pretty_time_diff(struct timespec *start, struct timespec *stop)
{
	struct string *s = crtes(100);
	int64_t diff_s = stop->tv_sec - start->tv_sec;
	int64_t diff_ns = stop->tv_nsec - start->tv_nsec;
	if (diff_ns < 0 && diff_s > 0) {
		diff_ns += INT64_C(1000000000);
		diff_s -= 1;
	}
	if (diff_s > 0) {
		str_appendf(s, "%.2f s", (double)diff_s + (double)diff_ns / 1000000000.0);
	} else if (diff_ns > 1000000) {
		str_appendf(s, "%.2f ms", (double)diff_ns / 1000000.0);
	} else if (diff_ns > 1000) {
		str_appendf(s, "%.2f us", (double)diff_ns / 1000.0);
	} else {
		str_appendf(s, "%" PRId64 " ns", diff_ns);
	}
	return s;
}
