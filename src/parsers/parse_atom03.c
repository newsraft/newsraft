#include <stdio.h>
#include "feedeater.h"

int
parse_atom03(XML_Parser *parser, struct string *feed_url, struct feed_parser_data *feed_data)
{
	int error = 1;
	status_write("Support for Atom 0.3 is under construction!");
	return error;
}
