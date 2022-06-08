#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS10
#include "update_feed/parse_feed/parse_xml_feed/parse_xml_feed.h"

// https://web.archive.org/web/20211106023928/https://web.resource.org/rss/1.0/spec

const struct xml_element_handler xml_rss10_handlers[] = {
	{NULL, XML_UNKNOWN_POS, NULL, NULL},
};
#endif // NEWSRAFT_FORMAT_SUPPORT_RSS10
