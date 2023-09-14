#include <string.h>
#include "newsraft.h"

static const char *test_cases[][2] = {
	{"The Devil's Advocate", "The Devil's Advocate"},
	{"Monsters,   Inc.",     "Monsters,   Inc."},
	{"The    Terminal",      "The    Terminal"},
	{"消失的她",             "     消失的她"},
	{"The Departed",         "The Departed           "},
	{"影",                   "\t \n \t 影 \t \r \t"},
	{"Black Swan",           "       Black Swan     "},
	{"El hoyo",              " El hoyo "},
	{NULL,                   NULL},
};

int
main(void)
{
	struct string *str = crtes(100);
	for (size_t i = 0; test_cases[i][0] != NULL; ++i) {
		cpyas(str, test_cases[i][1], strlen(test_cases[i][1]));
		trim_whitespace_from_string(str);
		if (strcmp(str->ptr, test_cases[i][0]) != 0) {
			free_string(str);
			return 1;
		}
	}
	free_string(str);
	return 0;
}
