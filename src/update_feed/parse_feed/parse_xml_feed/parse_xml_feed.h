#ifndef PARSE_XML_FEED_H
#define PARSE_XML_FEED_H
#include "update_feed/parse_feed/parse_feed.h"

enum parsing_error {
	PARSE_OKAY = 0,
	PARSE_FAIL_NOT_ENOUGH_MEMORY,
};

// Unknown position must have 0 value!
enum xml_position {
	XML_UNKNOWN_POS = 0,
	ATOM10_FEED,
	ATOM10_ENTRY,
	ATOM10_ID,
	ATOM10_TITLE,
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
	RSS20_CHANNEL,
	RSS20_ITEM,
	RSS20_GUID,
	RSS20_TITLE,
	RSS20_LINK,
	RSS20_DESCRIPTION,
	RSS20_PUBDATE,
	RSS20_LASTBUILDDATE,
	RSS20_AUTHOR,
	RSS20_CATEGORY,
	RSS20_COMMENTS,
	RSS20_TTL,
	RSS20_LANGUAGE,
	RSS20_GENERATOR,
	RSS20_WEBMASTER,
	RSS20_MANAGINGEDITOR,
	RSS20_SOURCE,
	RSSCONTENT_ENCODED,
	DC_TITLE,
	DC_DESCRIPTION,
	DC_CREATOR,
	DC_CONTRIBUTOR,
	DC_SUBJECT,
	MRSS_CONTENT,
	MRSS_THUMBNAIL,
	MRSS_DESCRIPTION,
	YANDEX_FULL_TEXT,
	YANDEX_COMMENT_TEXT,
	YANDEX_GENRE,
	YANDEX_BIND_TO,
	RSS11_CHANNEL,
	RSS11_ITEM,
	RSS11_TITLE,
	RSS11_LINK,
	RSS11_DESCRIPTION,
	RSS11_IMAGE,
	RSS11_URL,
	ATOM03_FEED,
	ATOM03_ENTRY,
	ATOM03_ID,
	ATOM03_TITLE,
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
	GEORSS_POINT,
	GEORSS_GML_POS,
};

struct xml_element_handler {
	const char *name;
	uint8_t bitpos;
	int8_t (*start_handle)(struct stream_callback_data *data, const XML_Char **atts);
	int8_t (*end_handle)(struct stream_callback_data *data);
};

// Functions common to parsers of all formats.
// See "xml-common.c" file for implementation.
bool we_are_inside_item(const struct stream_callback_data *data);
const char *get_value_of_attribute_key(const XML_Char **atts, const char *key);
bool copy_type_of_text_construct(struct string **dest, const XML_Char **atts);

#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM10
extern const struct xml_element_handler xml_atom10_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS20
extern const struct xml_element_handler xml_rss20_handlers[];
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
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS11
extern const struct xml_element_handler xml_rss11_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS10
extern const struct xml_element_handler xml_rss10_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS09
extern const struct xml_element_handler xml_rss09_handlers[];
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
