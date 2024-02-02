#include <string.h>
#include "newsraft.h"

struct serialize_test {
	const char *key;
	size_t key_len;
	const char *value;
	size_t value_len;
	const char *result;
};

static const struct serialize_test tests[] = {
	{"a=", 2, "test0",                                                        5, "\x1F" "a=test0"},
	{"b=", 2, "\x1F" "test1",                                                 6, "\x1F" "b=test1"},
	{"c=", 2, "te" "\x1F" "st2",                                              6, "\x1F" "c=test2"},
	{"d=", 2, "test3" "\x1F",                                                 6, "\x1F" "d=test3"},
	{"e=", 2, "\x1F\x1F" "test4",                                             7, "\x1F" "e=test4"},
	{"f=", 2, "te" "\x1F\x1F" "st5",                                          7, "\x1F" "f=test5"},
	{"g=", 2, "test6" "\x1F\x1F",                                             7, "\x1F" "g=test6"},
	{"h=", 2, "",                                                             0, ""},
	{"i=", 2, "\x1F\x1F\x1F\x1F\x1F",                                         5, ""},
	{"j=", 2, "\x1F" "test7" "\x1F\x1F",                                      8, "\x1F" "j=test7"},
	{"k=", 2, "\x1F" "te" "\x1F" "st8" "\x1F",                                8, "\x1F" "k=test8"},
	{"l=", 2,        "t" "\x1F" "e" "\x1F" "s" "\x1F" "t" "\x1F" "9",         9, "\x1F" "l=test9"},
	{"m=", 2, "\x1F" "t" "\x1F" "e" "\x1F" "s" "\x1F" "t" "\x1F" "A" "\x1F", 11, "\x1F" "m=testA"},
};

int
main(void)
{
	struct string *out = crtes(100);
	for (size_t i = 0; i < sizeof(tests) / sizeof(*tests); ++i) {
		serialize_array(&out, tests[i].key, tests[i].key_len, tests[i].value, tests[i].value_len);
		if (strcmp(out->ptr, tests[i].result) != 0) {
			fprintf(stderr, "Test %zu failed!\n", i);
			return 1;
		}
		empty_string(out);
	}
	return 0;
}
