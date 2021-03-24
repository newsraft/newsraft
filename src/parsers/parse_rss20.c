#include <stdio.h>
#include <stdlib.h>
#include "feedeater.h"
#include "parsers/parsers.h"

struct feed_entry *
parse_rss20(struct string *buf)
{
	struct feed_entry *feed = calloc(1, sizeof(struct feed_entry));
	status_write("rss20 under_construction");
	return feed;
}
