#include <locale.h>
#include <string.h>
#include "newsraft.h"

#define CHECK_UINT(A, B) if (A != B)            { fprintf(stderr, "%zu != %zu\n", A, B); return 1; }
#define CHECK_STR(A, B)  if (strcmp(A, B) != 0) { fprintf(stderr, "%s != %s\n",   A, B); return 1; }

int
main(void)
{
	setlocale(LC_ALL, "");

	set_db_path("./test-database");
	set_feeds_path("./feeds");
	load_config();
	db_init();

	parse_feeds_file();

	size_t feeds_count = 0;
	struct feed_entry **feeds = get_all_feeds(&feeds_count);

	if (feeds_count != 19) {
		fprintf(stderr, "Feeds count %zu != 18\n", feeds_count); return 1;
	}

	CHECK_STR(feeds[0]->link->ptr, "http://example.org/feed1.xml")
	CHECK_STR(feeds[1]->link->ptr, "http://example.org/feed2.xml")
	CHECK_STR(feeds[2]->link->ptr, "http://example.org/feed3.xml")
	CHECK_STR(feeds[3]->link->ptr, "http://example.org/feed4.xml")
	CHECK_STR(feeds[4]->link->ptr, "http://example.org/feed5.xml")

	CHECK_STR(feeds[3]->name->ptr, "Feed 4 title")
	CHECK_STR(feeds[4]->name->ptr, "Feed 5 title")
	CHECK_STR(feeds[6]->name->ptr, "Feed 7 title")

	CHECK_UINT(get_cfg_uint(&feeds[1]->cfg,   CFG_RELOAD_PERIOD), 60)
	CHECK_UINT(get_cfg_uint(&feeds[2]->cfg,   CFG_RELOAD_PERIOD), 60)
	CHECK_UINT(get_cfg_uint(&feeds[4]->cfg,   CFG_RELOAD_PERIOD), 120)
	CHECK_UINT(get_cfg_uint(&feeds[5]->cfg,   CFG_RELOAD_PERIOD), 180)
	CHECK_UINT(get_cfg_uint(&feeds[5]->cfg,   CFG_ITEM_LIMIT),    100)

	CHECK_UINT(get_cfg_uint(&feeds[6]->cfg,   CFG_RELOAD_PERIOD), 111)
	CHECK_UINT(get_cfg_uint(&feeds[6]->cfg,   CFG_ITEM_LIMIT),    222)
	CHECK_UINT(get_cfg_uint(&feeds[7]->cfg,   CFG_RELOAD_PERIOD), 333)
	CHECK_UINT(get_cfg_uint(&feeds[7]->cfg,   CFG_ITEM_LIMIT),    444)

	CHECK_STR(get_cfg_string(&feeds[8]->cfg,  CFG_PROXY)->ptr, "127.0.0.1")
	CHECK_STR(get_cfg_string(&feeds[9]->cfg,  CFG_PROXY)->ptr, "localhost")
	CHECK_STR(get_cfg_string(&feeds[10]->cfg, CFG_PROXY)->ptr, "localhost.localdomain")

	CHECK_STR(get_cfg_string(&feeds[11]->cfg, CFG_PROXY)->ptr, "127.0.0.1")
	CHECK_STR(get_cfg_string(&feeds[12]->cfg, CFG_PROXY)->ptr, "127.0.0.1")
	CHECK_STR(get_cfg_string(&feeds[13]->cfg, CFG_PROXY)->ptr, "127.0.0.1")
	CHECK_STR(get_cfg_string(&feeds[14]->cfg, CFG_PROXY)->ptr, "127.0.0.1")
	CHECK_STR(get_cfg_string(&feeds[15]->cfg, CFG_PROXY)->ptr, "127.0.0.1")
	CHECK_STR(get_cfg_string(&feeds[16]->cfg, CFG_PROXY)->ptr, "127.0.0.1")

	CHECK_UINT(get_cfg_uint(&feeds[17]->cfg,  CFG_RELOAD_PERIOD), 60)
	CHECK_UINT(get_cfg_uint(&feeds[17]->cfg,  CFG_ITEM_LIMIT),    100)
	CHECK_UINT(get_cfg_uint(&feeds[18]->cfg,  CFG_RELOAD_PERIOD), 120)
	CHECK_UINT(get_cfg_uint(&feeds[18]->cfg,  CFG_ITEM_LIMIT),    200)

	return 0;
}
