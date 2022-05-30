#ifndef PARSE_XML_FEED_H
#define PARSE_XML_FEED_H
#include "update_feed/parse_feed/parse_feed.h"

enum parsing_error {
	PARSE_OKAY = 0,
	PARSE_FAIL_NOT_ENOUGH_MEMORY,
};

struct xml_element_handler {
	const char *name;
	int32_t bitpos;
	int8_t (*start_handle)(struct stream_callback_data *data, const XML_Char **atts);
	int8_t (*end_handle)(struct stream_callback_data *data);
};

// Functions common to parsers of all formats.
// See "xml-common.c" file for implementation.
bool we_are_inside_item(const struct stream_callback_data *data);
const char *get_value_of_attribute_key(const XML_Char **atts, const char *key);
bool copy_type_of_text_construct(struct string **dest, const XML_Char **atts);

#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM10
enum atom10_position {
	ATOM10_NONE = 0,
	ATOM10_ENTRY = 1,
	ATOM10_ID = 2,
	ATOM10_TITLE = 4,
	ATOM10_SUMMARY = 8,
	ATOM10_CONTENT = 16,
	ATOM10_PUBLISHED = 32,
	ATOM10_UPDATED = 64,
	ATOM10_AUTHOR = 128,
	ATOM10_NAME = 256,
	ATOM10_URI = 512,
	ATOM10_EMAIL = 1024,
	ATOM10_SUBTITLE = 2048,
	ATOM10_GENERATOR = 4096,
};
extern const struct xml_element_handler xml_atom10_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS20
enum rss20_position {
	RSS20_NONE = 0,
	RSS20_ITEM = 1,
	RSS20_GUID = 2,
	RSS20_TITLE = 4,
	RSS20_LINK = 8,
	RSS20_DESCRIPTION = 16,
	RSS20_PUBDATE = 32,
	RSS20_LASTBUILDDATE = 64,
	RSS20_AUTHOR = 128,
	RSS20_CATEGORY = 256,
	RSS20_COMMENTS = 512,
	RSS20_TTL = 1024,
	RSS20_LANGUAGE = 2048,
	RSS20_GENERATOR = 4096,
	RSS20_WEBMASTER = 8192,
	RSS20_MANAGINGEDITOR = 16384,
	RSS20_SOURCE = 32768,
};
extern const struct xml_element_handler xml_rss20_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSSCONTENT
enum rsscontent_position {
	RSSCONTENT_NONE = 0,
	RSSCONTENT_ENCODED = 1,
};
extern const struct xml_element_handler xml_rsscontent_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_DUBLINCORE
enum dc_position {
	DC_NONE = 0,
	DC_TITLE = 1,
	DC_DESCRIPTION = 2,
	DC_CREATOR = 4,
	DC_CONTRIBUTOR = 8,
	DC_SUBJECT = 16,
};
extern const struct xml_element_handler xml_dublincore_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_MEDIARSS
enum mrss_position {
	MRSS_NONE = 0,
	MRSS_CONTENT = 1,
	MRSS_THUMBNAIL = 2,
	MRSS_DESCRIPTION = 4,
};
extern const struct xml_element_handler xml_mediarss_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_YANDEX
enum yandex_position {
	YANDEX_NONE = 0,
	YANDEX_FULL_TEXT = 1,
	YANDEX_GENRE = 2,
	YANDEX_COMMENT_TEXT = 4,
	YANDEX_BIND_TO = 8,
};
extern const struct xml_element_handler xml_yandex_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS11
enum rss11_position {
	RSS11_NONE = 0,
	RSS11_ITEM = 1,
	RSS11_TITLE = 2,
	RSS11_LINK = 4,
	RSS11_DESCRIPTION = 8,
	RSS11_IMAGE = 16,
	RSS11_URL = 32,
};
extern const struct xml_element_handler xml_rss11_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM03
enum atom03_position {
	ATOM03_NONE = 0,
	ATOM03_ENTRY = 1,
	ATOM03_ID = 2,
	ATOM03_TITLE = 4,
	ATOM03_SUMMARY = 8,
	ATOM03_CONTENT = 16,
	ATOM03_ISSUED = 32,
	ATOM03_MODIFIED = 64,
	ATOM03_AUTHOR = 128,
	ATOM03_NAME = 256,
	ATOM03_URL = 512,
	ATOM03_EMAIL = 1024,
	ATOM03_TAGLINE = 2048,
	ATOM03_GENERATOR = 4096,
};
extern const struct xml_element_handler xml_atom03_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_GEORSS
enum georss_position {
	GEORSS_NONE = 0,
	GEORSS_POINT = 1,
};
extern const struct xml_element_handler xml_georss_handlers[];
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_GEORSS_GML
enum georss_gml_position {
	GEORSS_GML_NONE = 0,
	GEORSS_GML_POS = 1,
};
extern const struct xml_element_handler xml_georss_gml_handlers[];
#endif
#endif // PARSE_XML_FEED_H
