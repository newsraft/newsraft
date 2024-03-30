#include <locale.h>
#include <string.h>
#include "newsraft.h"

struct feed_test {
	const char *link;
	const char *name;
	size_t reload_period;
	size_t item_limit;
	const char *proxy;
};

struct feed_test feed_tests[] = {
	{"http://example.org/feed1.xml",            NULL,   0,   0,                      ""},
	{"http://example.org/feed2.xml",            NULL,  60,   0,                      ""},
	{"http://example.org/feed3.xml",            NULL,  60,   0,                      ""},
	{"http://example.org/feed4.xml",  "Feed 4 title",   0,   0,                      ""},
	{"http://example.org/feed5.xml",  "Feed 5 title", 120,   0,                      ""},
	{"http://example.org/feed6.xml",            NULL, 180, 100,                      ""},
	{"http://example.org/feed7.xml",  "Feed 7 title", 111, 222,                      ""},
	{"http://example.org/feed8.xml",            NULL, 333, 444,                      ""},
	{"http://example.org/feed9.xml",            NULL,   0,   0,             "127.0.0.1"},
	{"$(curl foo://bar.baz/feed10)",            NULL,   0,   0,             "localhost"},
	{"$(curl foo://bar.baz/feed11)",            NULL,   0,   0, "localhost.localdomain"},
	{"$(curl foo://bar.baz/feed12)",            NULL,   0,   0,             "127.0.0.1"},
	{"$(curl foo://bar.baz/feed13)",            NULL,   0,   0,             "127.0.0.1"},
	{"$(curl foo://bar.baz/feed14)",            NULL,   0,   0,             "127.0.0.1"},
	{"$(curl foo://bar.baz/feed15)",            NULL,   0,   0,             "127.0.0.1"},
	{"$(curl foo://bar.baz/feed16)",            NULL,   0,   0,             "127.0.0.1"},
	{"$(curl foo://bar.baz/feed17)",            NULL,   0,   0,             "127.0.0.1"},
	{"$(curl foo://bar.baz/feed18)",            NULL,  60, 100,                      ""},
	{"$(curl foo://bar.baz/feed19)",            NULL, 120, 200,                      ""},
	{"$(curl foo://bar.baz/feed20)",            NULL,   0,   0,                      ""},
	{"$(curl foo://bar.baz/feed21)", "Feed 21 title",   0,   0,                      ""},
	{"$(curl foo://bar.baz/feed22)",            NULL, 720,   0,                      ""},
	{"$(curl foo://bar.baz/feed23)",            NULL,   0,   0,                      ""},
	{"$(curl foo://bar.baz/feed24)",            NULL, 360,   0,                      ""},
	{"$(curl foo://bar.baz/feed25)",            NULL, 180,   0,                      ""},
	{"$(curl foo://bar.baz/feed26)",            NULL, 360,   0,                      ""},
};

int
main(void)
{
	int status = 0;
	setlocale(LC_ALL, "");

	set_db_path("./test-database");
	set_feeds_path("./feeds");
	load_config();
	db_init();

	parse_feeds_file();

	size_t feeds_count = 0;
	struct feed_entry **feeds = get_all_feeds(&feeds_count);

	if (feeds_count != 26) {
		fprintf(stderr, "Feeds count %zu != %zu\n", feeds_count, 26);
		return 1;
	}

	size_t i;
	for (i = 0; status == 0 && i < feeds_count; ++i) {
		size_t reload_period = get_cfg_uint(&feeds[i]->cfg, CFG_RELOAD_PERIOD);
		size_t item_limit = get_cfg_uint(&feeds[i]->cfg, CFG_ITEM_LIMIT);
		const char *proxy = get_cfg_string(&feeds[i]->cfg, CFG_PROXY)->ptr;

		if (strcmp(feeds[i]->link->ptr, feed_tests[i].link) != 0) {
			fprintf(stderr, "Link: %s != %s\n", feeds[i]->link->ptr, feed_tests[i].link);
			status = 1;
		}
		if (feeds[i]->name != NULL && feed_tests[i].name != NULL && strcmp(feeds[i]->name->ptr, feed_tests[i].name) != 0) {
			fprintf(stderr, "Name: %s != %s\n", feeds[i]->name->ptr, feed_tests[i].name);
			status = 1;
		}
		if (reload_period != feed_tests[i].reload_period) {
			fprintf(stderr, "Reload period: %zu != %zu\n", reload_period, feed_tests[i].reload_period);
			status = 1;
		}
		if (item_limit != feed_tests[i].item_limit) {
			fprintf(stderr, "Item limit: %zu != %zu\n", item_limit, feed_tests[i].item_limit);
			status = 1;
		}
		if (strcmp(proxy, feed_tests[i].proxy) != 0) {
			fprintf(stderr, "Proxy: %s != %s\n", proxy, feed_tests[i].proxy);
			status = 1;
		}
	}

	if (status != 0) {
		fprintf(stderr, "Feed %zu failed!\n", i);
	}

	return status;
}
