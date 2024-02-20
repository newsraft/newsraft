#include <string.h>
#include "newsraft.h"

static const char *test_cases[][2] = {
	// Input                    Output
	{"King Stephen",            "King Stephen"},
	{"\t \n Hensonn",           " Hensonn"},
	{"OFFL1NX \r  \n",          "OFFL1NX "},
	{"  \n Vakhtang \n",        " Vakhtang "},
	{"Hollywood\n \n\t\tBurns", "Hollywood Burns"},
	{NULL,                      NULL},
};

int
main(void)
{
	struct string *str = crtes(100);
	for (size_t i = 0; test_cases[i][0] != NULL; ++i) {
		cpyas(&str, test_cases[i][0], strlen(test_cases[i][0]));
		inlinefy_string(str);
		if (strcmp(str->ptr, test_cases[i][1]) != 0) {
			free_string(str);
			return 1;
		}
	}
	free_string(str);
	return 0;
}
