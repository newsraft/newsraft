#include <string.h>
#include "newsraft.h"

static const char *test_cases[][2] = {
	{"Charlie and the Chocolate Factory",  ")c+Dbu!i%KCp<+C83v{/3V6p5c${H0M`6=t.UbNq 0b(%xSX<~63v/;DkkHI%<df"},
	{"He Died With A Felafel In His Hand", "8L}b+3(q$2Vx+jJf0X, $a<q-k]UP%5YD,hQrlHv9Z|l_>-t*~9%cMmWmC~-tW6s"},
	{"And Justice for All",                "6,)T(sc}_B{T=}D=l|whQrJ6g;i59HN:QT&&f}.?mO7;/<r^?18uH3F*}_sjG&X3"},
	{NULL,                                 NULL},
};

int
main(void)
{
	struct string *str = crtes(100);
	for (size_t i = 0; test_cases[i][0] != NULL; ++i) {
		newsraft_simple_hash(&str, test_cases[i][0]);
		if (strcmp(str->ptr, test_cases[i][1]) != 0) {
			printf("%s != %s\n", str->ptr, test_cases[i][1]);
			return 1;
		}
	}
	free_string(str);
	return 0;
}
