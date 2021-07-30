#include "feedeater.h"

/* Generic parsing is based on namespaced tags processing only.
 * Thus if tag does belong to some namespace and feedeater recognizes it,
 * it will be parsed by functions listed in parse-namespace.c */

void XMLCALL
elem_generic_start(void *userData, const XML_Char *name, const XML_Char **atts)
{
	struct parser_data *data = userData;
	++(data->depth);
	process_namespaced_tag_start(userData, name, atts);
}

void XMLCALL
elem_generic_finish(void *userData, const XML_Char *name)
{
	struct parser_data *data = userData;
	--(data->depth);
	value_strip_whitespace(data->value, &data->value_len);
	process_namespaced_tag_end(userData, name);
}
