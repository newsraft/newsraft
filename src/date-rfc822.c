#include <time.h>

time_t
parse_date_rfc822(char *date_str)
{
	struct tm t = {0};
	if (strptime(date_str, "%a, %d %b %Y %H:%M:%S %z", &t) != NULL) {
		return mktime(&t);
	}
	return 0;
}
