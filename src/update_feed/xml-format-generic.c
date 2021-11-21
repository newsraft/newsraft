#include "feedeater.h"
#include "update_feed/update_feed.h"

/* Generic parsing is based on processing of namespace tags.
 * Thus if tag does belong to some namespace and feedeater recognizes it,
 * it will be parsed by functions listed in xml-namespace.c */

void XMLCALL
parse_generic_element_start(struct parser_data *data, const XML_Char *name, const XML_Char **atts)
{
	parse_namespace_element_start(data, name, atts);
}

void XMLCALL
parse_generic_element_end(struct parser_data *data, const XML_Char *name)
{
	parse_namespace_element_end(data, name);
}
