#include <string.h>
#include "newsraft.h"

static const char *test_cases[][2] = {
	// Input                          Output
	{"a",                             "YQ=="},
	{"z",                             "eg=="},
	{"0",                             "MA=="},
	{"9",                             "OQ=="},
	{"Saltburn",                      "U2FsdGJ1cm4="},
	{"I Care a Lot",                  "SSBDYXJlIGEgTG90"},
	{"Suspiria",                      "U3VzcGlyaWE="},
	{"Chinese Coffee",                "Q2hpbmVzZSBDb2ZmZWU="},
	{"The Killing of a Sacred Deer",  "VGhlIEtpbGxpbmcgb2YgYSBTYWNyZWQgRGVlcg=="},
	{"Whiplash",                      "V2hpcGxhc2g="},
	{"Scent of a Woman",              "U2NlbnQgb2YgYSBXb21hbg=="},
	{"I'm Thinking of Ending Things", "SSdtIFRoaW5raW5nIG9mIEVuZGluZyBUaGluZ3M="},
	{"Gone Girl",                     "R29uZSBHaXJs"},
	{"Requiem for a Dream",           "UmVxdWllbSBmb3IgYSBEcmVhbQ=="},
	{"Midsommar",                     "TWlkc29tbWFy"},
	{"The Banshees of Inisherin",     "VGhlIEJhbnNoZWVzIG9mIEluaXNoZXJpbg=="},
	{NULL,                            NULL},
};

int
main(void)
{
	size_t errors = 0;
	for (size_t i = 0; test_cases[i][0] != NULL; ++i) {
		struct string *out = newsraft_base64_encode((uint8_t *)test_cases[i][0], strlen(test_cases[i][0]));
		if (strcmp(out->ptr, test_cases[i][1]) != 0) {
			fprintf(stderr, "\"%s\" != \"%s\"\n", out->ptr, test_cases[i][1]);
			errors += 1;
		}
		free_string(out);
	}
	return errors ? 1 : 0;
}
