#ifndef PARSE_XML_FEED_H
#define PARSE_XML_FEED_H
#include "update_feed/update_feed.h"

enum parsing_error {
	PARSE_OKAY = 0,
	PARSE_FAIL_NOT_ENOUGH_MEMORY,
};

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
	int8_t (*start_handle)(struct stream_callback_data *data, const XML_Char **atts);
	int8_t (*end_handle)(struct stream_callback_data *data);
};

// Functions common to parsers of all formats.
// See "xml-common.c" file for implementation.
const char *get_value_of_attribute_key(const XML_Char **attrs, const char *key);
bool serialize_attribute(struct string **dest, const XML_Char **attrs, const char *attr_key, const char *prefix, size_t prefix_len);
int8_t generic_item_starter(struct stream_callback_data *data, const XML_Char **attrs);
int8_t generic_item_ender(struct stream_callback_data *data);
int8_t generic_guid_end(struct stream_callback_data *data);
int8_t generic_title_end(struct stream_callback_data *data);
int8_t generic_plain_content_end(struct stream_callback_data *data);
int8_t generic_html_content_end(struct stream_callback_data *data);
int8_t generic_category_end(struct stream_callback_data *data);
int8_t generator_start(struct stream_callback_data *data, const XML_Char **attrs);
int8_t generator_end(struct stream_callback_data *data);

#ifndef NEWSRAFT_DISABLE_FORMAT_ATOM10
extern const struct xml_element_handler xml_atom10_handlers[];
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_RSS
extern const struct xml_element_handler xml_rss_handlers[];
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_RSSCONTENT
extern const struct xml_element_handler xml_rsscontent_handlers[];
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_DUBLINCORE
extern const struct xml_element_handler xml_dublincore_handlers[];
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_MEDIARSS
extern const struct xml_element_handler xml_mediarss_handlers[];
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_XHTML
extern const struct xml_element_handler xml_xhtml_handlers[];
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_YANDEX
extern const struct xml_element_handler xml_yandex_handlers[];
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_RBCNEWS
extern const struct xml_element_handler xml_rbcnews_handlers[];
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_GEORSS
extern const struct xml_element_handler xml_georss_handlers[];
#endif
#ifndef NEWSRAFT_DISABLE_FORMAT_GEORSS_GML
extern const struct xml_element_handler xml_georss_gml_handlers[];
#endif
#endif // PARSE_XML_FEED_H
