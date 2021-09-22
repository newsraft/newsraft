#include "feedeater.h"

/* Generic parsing is based on processing of namespace tags.
 * Thus if tag does belong to some namespace and feedeater recognizes it,
 * it will be parsed by functions listed in parse-namespace.c */

void XMLCALL
parse_generic_element_beginning(void *userData, const XML_Char *name, const XML_Char **atts)
{
	struct parser_data *data = userData;
	++(data->depth);
	parse_namespace_element_beginning(userData, name, atts);
}

void XMLCALL
parse_generic_element_end(void *userData, const XML_Char *name)
{
	struct parser_data *data = userData;
	--(data->depth);
	parse_namespace_element_end(userData, name);
}
