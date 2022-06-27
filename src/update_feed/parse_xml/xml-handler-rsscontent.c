#include "update_feed/parse_xml/parse_xml_feed.h"

// https://web.resource.org/rss/1.0/modules/content/

const struct xml_element_handler xml_rsscontent_handlers[] = {
	{"encoded", XML_UNKNOWN_POS, NULL, &generic_html_content_end},
	{NULL,      XML_UNKNOWN_POS, NULL, NULL},
};
