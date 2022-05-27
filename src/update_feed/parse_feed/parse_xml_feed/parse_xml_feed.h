#ifndef PARSE_XML_FEED_H
#define PARSE_XML_FEED_H
#include <tidy.h>
#include <tidybuffio.h>
#include "update_feed/parse_feed/parse_feed.h"

#define XML_NAMESPACE_SEPARATOR ':'

enum xml_format {
#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM10
	ATOM10_FORMAT = 0,
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS20
	RSS20_FORMAT,
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSSCONTENT
	RSSCONTENT_FORMAT,
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_DUBLINCORE
	DUBLINCORE_FORMAT,
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_MEDIARSS
	MEDIARSS_FORMAT,
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_YANDEX
	YANDEX_FORMAT,
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_RSS11
	RSS11_FORMAT,
	RSS11_2_FORMAT, // don't use it
	RSS11_3_FORMAT, // don't use it
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_ATOM03
	ATOM03_FORMAT,
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_GEORSS
	GEORSS_FORMAT,
#endif
#ifdef NEWSRAFT_FORMAT_SUPPORT_GEORSS_GML
	GEORSS_GML_FORMAT,
#endif
	XML_FORMATS_COUNT,
};

struct xml_namespace {
	struct string *name;
	struct string *uri;
};

struct xml_namespace_stack {
	uint16_t top;
	size_t lim;
	struct xml_namespace *buf;
};

struct xml_default_namespace {
	struct string *uri;
	struct xml_default_namespace *next;
};

struct xml_data {
	struct string *value;
	int depth;
	struct xml_default_namespace *def_ns;
	struct xml_namespace_stack namespaces;
	const struct string *feed_url;
	struct getfeed_feed *feed;
	intmax_t xml_pos[XML_FORMATS_COUNT];
	int8_t default_handler;
	TidyDoc tidy_doc;
	TidyBuffer draft_buffer;
	enum update_error error;
};

struct xml_element_handler {
	const char *name;
	intmax_t bitpos;
	void (*start_handle)(struct xml_data *data, const TidyAttr attrs);
	void (*end_handle)(struct xml_data *data, const TidyAttr attrs);
};

bool prepend_default_namespace(struct xml_default_namespace **first_def_ns, const char *uri_to_prepend, size_t uri_len);
void discard_default_namespace(struct xml_default_namespace **first_def_ns);
void free_default_namespaces(struct xml_default_namespace *first_def_ns);

bool add_namespace_to_stack(struct xml_namespace_stack *stack, const char *name, const char *uri);
void pop_namespace_from_stack(struct xml_namespace_stack *stack);
const struct string *find_namespace_uri_by_its_name(const struct xml_namespace_stack *namespaces, const char *name, size_t name_len);
void free_namespace_stack(struct xml_namespace_stack *stack);

// Functions common to parsers of all formats.
// See "xml-common.c" file for implementation.
bool we_are_inside_item(const struct xml_data *data);
const char *get_value_of_attribute_key(const TidyAttr attrs, const char *key);
bool copy_type_of_text_construct(struct string **dest, const TidyAttr attrs);

// Element handlers

void parse_element_start(struct xml_data *data, const struct string *namespace_uri, const char *name, const TidyAttr attrs);
void parse_element_end(struct xml_data *data, const struct string *namespace_uri, const char *name, const TidyAttr attrs);

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
	RSS20_LANGUAGE = 1024,
	RSS20_GENERATOR = 2048,
	RSS20_WEBMASTER = 4096,
	RSS20_MANAGINGEDITOR = 8192,
	RSS20_SOURCE = 16384,
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
