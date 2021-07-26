#include <time.h>

time_t
parse_date_rfc3339(char *date_str, size_t date_len)
{
	struct tm t = {0};
	if (date_len != 0 && date_str[date_len - 1] == 'Z') {
		if (strptime(date_str, "%Y-%m-%dT%H:%M:%SZ", &t) != NULL) {
			return mktime(&t);
		}
	} else {
		if (strptime(date_str, "%Y-%m-%dT%H:%M:%S%z", &t) != NULL) {
			return mktime(&t);
		}
	}
	return 0;
}
