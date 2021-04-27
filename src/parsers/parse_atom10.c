#include <stdio.h>
#include "feedeater.h"

int
parse_atom10(XML_Parser *parser, char *feed_url, struct feed_parser_data *feed_data)
{
	int error = 1;
	status_write("Support for Atom 1.0 is under construction!");
	return error;
}
