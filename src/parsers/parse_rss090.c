#include <stdio.h>
#include "feedeater.h"

int
parse_rss090(XML_Parser *parser, char *feed_url, struct feed_parser_data *feed_data)
{
	int error = 1;
	status_write("Support for RSS 0.90 is under construction!");
	return error;
}
