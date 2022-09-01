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
	ATOM10_SUMMARY,
	ATOM10_CONTENT,
	ATOM10_PUBLISHED,
	ATOM10_UPDATED,
	ATOM10_AUTHOR,
	ATOM10_NAME,
	ATOM10_URI,
	ATOM10_EMAIL,
	ATOM10_SUBTITLE,
	ATOM10_GENERATOR,
	RSS20_LINK,
	RSS20_DESCRIPTION,
	RSS20_PUBDATE,
	RSS20_LASTBUILDDATE,
	RSS20_AUTHOR,
	RSS20_COMMENTS,
	RSS20_TTL,
	RSS20_GENERATOR,
	RSS20_WEBMASTER,
	RSS20_MANAGINGEDITOR,
	RSS20_SOURCE,
	DC_TITLE,
	DC_CREATOR,
	DC_CONTRIBUTOR,
	MEDIARSS_CONTENT,
	ATOM03_SUMMARY,
	ATOM03_CONTENT,
	ATOM03_ISSUED,
	ATOM03_MODIFIED,
	ATOM03_AUTHOR,
	ATOM03_NAME,
	ATOM03_URL,
	ATOM03_EMAIL,
	ATOM03_TAGLINE,
	ATOM03_GENERATOR,
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

#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM10
extern const struct xml_element_handler xml_atom10_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS
extern const struct xml_element_handler xml_rss_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSSCONTENT
extern const struct xml_element_handler xml_rsscontent_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_DUBLINCORE
extern const struct xml_element_handler xml_dublincore_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_MEDIARSS
extern const struct xml_element_handler xml_mediarss_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_YANDEX
extern const struct xml_element_handler xml_yandex_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RBCNEWS
extern const struct xml_element_handler xml_rbcnews_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM03
extern const struct xml_element_handler xml_atom03_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_GEORSS
extern const struct xml_element_handler xml_georss_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_GEORSS_GML
extern const struct xml_element_handler xml_georss_gml_handlers[];
#endif
#endif // PARSE_XML_FEED_H
