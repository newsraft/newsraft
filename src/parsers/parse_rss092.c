#include <stdio.h>
#include "feedeater.h"

int
parse_rss092(XML_Parser *parser, struct string *feed_url, struct feed_parser_data *feed_data)
{
	int error = 1;
	status_write("Support for RSS 0.92 is under construction!");
	return error;
}
