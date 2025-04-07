#ifndef PARSE_XML_FEED_H
#define PARSE_XML_FEED_H
#include "newsraft.h"

// Unknown position must have 0 value!
enum xml_position {
	XML_UNKNOWN_POS = 0,
	GENERIC_FEED,
	GENERIC_ITEM,
	ATOM_AUTHOR,
	MEDIARSS_CONTENT,
};

struct xml_element_handler {
	const char *name;
	uint8_t bitpos;
	void (*start_handle)(struct feed_update_state *data, const XML_Char **atts);
	void (*end_handle)(struct feed_update_state *data);
};
#endif // PARSE_XML_FEED_H
