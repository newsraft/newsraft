#include "update_feed/parse_xml/parse_xml_feed.h"

const struct xml_element_handler xml_rbcnews_handlers[] = {
	{"full-text", XML_UNKNOWN_POS, NULL, &generic_plain_content_end},
	{"tag",       XML_UNKNOWN_POS, NULL, &generic_category_end},
	{NULL,        XML_UNKNOWN_POS, NULL, NULL},
};
