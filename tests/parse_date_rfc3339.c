#include "newsraft.h"

int
main(void)
{
	if (parse_date_rfc3339("1996-12-19T16:39:57-08:00") != 851031597) return 1;
	if (parse_date_rfc3339("1990-12-31T23:59:60Z")      != 662677200) return 1;
	if (parse_date_rfc3339("1990-12-31T15:59:60-08:00") != 662677200) return 1;
	return 0;
}
