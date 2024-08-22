#include <string.h>
#include "newsraft.h"

static const char *test_cases[][3] = {
	// Base                            Relative            Result
	{"http://example.org",             "/",                "http://example.org/"},
	{"http://example.org/",            "/",                "http://example.org/"},
	{"http://example.org/test",        "/",                "http://example.org/"},
	{"http://example.org",             "..",               "http://example.org/"},
	{"http://example.org/",            "..",               "http://example.org/"},
	{"http://example.org/test",        "..",               "http://example.org/"},
	{"http://example.org",             "../..",            "http://example.org/"},
	{"http://example.org/",            "../..",            "http://example.org/"},
	{"http://example.org/test",        "../..",            "http://example.org/"},
	{"http://example.org",             "../../..",         "http://example.org/"},
	{"http://example.org/",            "../../..",         "http://example.org/"},
	{"http://example.org/test",        "../../..",         "http://example.org/"},
	{"http://example.org/root",        "/",                "http://example.org/"},
	{"http://example.org/root/",       "/",                "http://example.org/"},
	{"http://example.org/root/test",   "/",                "http://example.org/"},
	{"http://example.org/root",        "..",               "http://example.org/"},
	{"http://example.org/root/",       "..",               "http://example.org/"},
	{"http://example.org/root/test",   "..",               "http://example.org/"},
	{"http://example.org/root",        "../..",            "http://example.org/"},
	{"http://example.org/root/",       "../..",            "http://example.org/"},
	{"http://example.org/root/test",   "../..",            "http://example.org/"},
	{"http://example.org/root",        "../../..",         "http://example.org/"},
	{"http://example.org/root/",       "../../..",         "http://example.org/"},
	{"http://example.org/root/test",   "../../..",         "http://example.org/"},
	{"http://example.org/root/q",      "/",                "http://example.org/"},
	{"http://example.org/root/q/",     "/",                "http://example.org/"},
	{"http://example.org/root/q/test", "/",                "http://example.org/"},
	{"http://example.org/root/q",      "..",               "http://example.org/"},
	{"http://example.org/root/q/",     "..",               "http://example.org/root/"},
	{"http://example.org/root/q/test", "..",               "http://example.org/root/"},
	{"http://example.org/root/q",      "../..",            "http://example.org/"},
	{"http://example.org/root/q/",     "../..",            "http://example.org/"},
	{"http://example.org/root/q/test", "../..",            "http://example.org/"},
	{"http://example.org/root/q",      "../../..",         "http://example.org/"},
	{"http://example.org/root/q/",     "../../..",         "http://example.org/"},
	{"http://example.org/root/q/test", "../../..",         "http://example.org/"},
	{"http://example.org",             "index.html",       "http://example.org/index.html"},
	{"http://example.org/" ,           "index.html",       "http://example.org/index.html"},
	{"http://example.org/test",        "index.html",       "http://example.org/index.html"},
	{"http://example.org",             "/index.html",      "http://example.org/index.html"},
	{"http://example.org/" ,           "/index.html",      "http://example.org/index.html"},
	{"http://example.org/test",        "/index.html",      "http://example.org/index.html"},
	{"http://example.org",             "../index.html",    "http://example.org/index.html"},
	{"http://example.org/" ,           "../index.html",    "http://example.org/index.html"},
	{"http://example.org/test",        "../index.html",    "http://example.org/index.html"},
	{"http://example.org",             "../../index.html", "http://example.org/index.html"},
	{"http://example.org/" ,           "../../index.html", "http://example.org/index.html"},
	{"http://example.org/test",        "../../index.html", "http://example.org/index.html"},
	{"http://example.org/root",        "index.html",       "http://example.org/index.html"},
	{"http://example.org/root/" ,      "index.html",       "http://example.org/root/index.html"},
	{"http://example.org/root/test",   "index.html",       "http://example.org/root/index.html"},
	{"http://example.org/root",        "/index.html",      "http://example.org/index.html"},
	{"http://example.org/root/" ,      "/index.html",      "http://example.org/index.html"},
	{"http://example.org/root/test",   "/index.html",      "http://example.org/index.html"},
	{"http://example.org/root",        "../index.html",    "http://example.org/index.html"},
	{"http://example.org/root/" ,      "../index.html",    "http://example.org/index.html"},
	{"http://example.org/root/test",   "../index.html",    "http://example.org/index.html"},
	{"http://example.org/root",        "../../index.html", "http://example.org/index.html"},
	{"http://example.org/root/" ,      "../../index.html", "http://example.org/index.html"},
	{"http://example.org/root/test",   "../../index.html", "http://example.org/index.html"},
	{"http://example.org/root/q",      "index.html",       "http://example.org/root/index.html"},
	{"http://example.org/root/q/" ,    "index.html",       "http://example.org/root/q/index.html"},
	{"http://example.org/root/q/test", "index.html",       "http://example.org/root/q/index.html"},
	{"http://example.org/root/q",      "/index.html",      "http://example.org/index.html"},
	{"http://example.org/root/q/" ,    "/index.html",      "http://example.org/index.html"},
	{"http://example.org/root/q/test", "/index.html",      "http://example.org/index.html"},
	{"http://example.org/root/q",      "../index.html",    "http://example.org/index.html"},
	{"http://example.org/root/q/" ,    "../index.html",    "http://example.org/root/index.html"},
	{"http://example.org/root/q/test", "../index.html",    "http://example.org/root/index.html"},
	{"http://example.org/root/q",      "../../index.html", "http://example.org/index.html"},
	{"http://example.org/root/q/" ,    "../../index.html", "http://example.org/index.html"},
	{"http://example.org/root/q/test", "../../index.html", "http://example.org/index.html"},
	{NULL,                             NULL,               NULL},
};

int
main(void)
{
	int status = 0;

	for (size_t i = 0; test_cases[i][0] != NULL; ++i) {
		char *full_url = complete_url(test_cases[i][0], test_cases[i][1]);
		if (strcmp(full_url, test_cases[i][2]) != 0) {
			fprintf(stderr, "Relative %zu: \"%s\" != \"%s\"\n", i, full_url, test_cases[i][2]);
			status = 1;
		}
		free(full_url);
	}

	const char *abs = "https://codeberg.org/newsraft/newsraft";
	for (size_t i = 0; test_cases[i][0] != NULL; ++i) {
		char *full_url = complete_url(test_cases[i][0], abs);
		if (strcmp(full_url, abs) != 0) {
			fprintf(stderr, "Absolute %zu: \"%s\" != \"%s\"\n", i, full_url, abs);
			status = 1;
		}
		free(full_url);
	}

	return status;
}
