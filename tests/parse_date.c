#include "newsraft.h"

static bool
test_date(const char *date, int64_t true_time)
{
	int64_t test_time = parse_date(date, false);
	if (test_time == true_time) {
		return true;
	} else {
		fprintf(stderr, "Mismatch for %s: %ld != %ld\n", date, test_time, true_time);
		return false;
	}
}

int
main(void)
{
	if (test_date("1996-12-19T16:39:57-08:00",        851042397) == false) return 1;
	if (test_date("1990-12-31T23:59:60Z",             662688000) == false) return 1;
	if (test_date("1990-12-31T15:59:60-08:00",        662688000) == false) return 1;
	if (test_date("Sun, 05 May 2024 17:53:44 +0200", 1714924424) == false) return 1;
	if (test_date("1978-07-03", /* YYYY-MM-DD */      268272000) == false) return 1;
	if (test_date("1978/07/03", /* YYYY/MM/DD */      268272000) == false) return 1;
	return 0;
}
