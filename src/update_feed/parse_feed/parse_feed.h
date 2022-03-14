#ifndef GETFEED_H
#define GETFEED_H
#include <tidy.h>
#include <cjson/cJSON.h>
#include "update_feed/update_feed.h"

#define XML_NAMESPACE_SEPARATOR ':'
#define COUNTOF(A) (sizeof(A) / sizeof(*A))
#define ISWHITESPACE(A) (((A)==' ')||((A)=='\n')||((A)=='\t')||((A)=='\v')||((A)=='\f')||((A)=='\r'))

enum update_error {
	PARSE_OKAY = 0,
	PARSE_FAIL_NOT_ENOUGH_MEMORY,
};

struct xml_namespace {
	struct string *name;
	struct string *uri;
};

struct xml_namespace_stack {
	uint16_t top;
	size_t lim;
	struct xml_namespace *buf;
	struct string *defaultns;
};

struct xml_data {
	struct string *value;
	int depth;
	struct xml_namespace_stack namespaces;
	const struct string *feed_url;
	struct getfeed_feed *feed;
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
	int32_t rss20_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
	int16_t atom10_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
	int8_t dc_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS10CONTENT
	int8_t rss10content_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_YANDEX
	int8_t yandex_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM03
	int16_t atom03_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS11
	int8_t rss11_pos;
#endif
	void (*start_handler)(struct xml_data *data, const char *name, const TidyAttr atts);
	void (*end_handler)(struct xml_data *data, const char *name);
	enum update_error error;
};

struct json_data {
	struct getfeed_feed *feed;
};

bool add_namespace_to_stack(struct xml_namespace_stack *stack, const char *name, const char *uri);
void pop_namespace_from_stack(struct xml_namespace_stack *stack);
void free_namespace_stack(struct xml_namespace_stack *stack);

struct getfeed_feed *parse_xml_feed(const struct string *feed_buf);
struct getfeed_feed *parse_json_feed(const struct string *feed_buf);

const char *get_value_of_attribute_key(const TidyAttr attrs, const char *key);
bool we_are_inside_item(const struct xml_data *data);

// feed bucket functions
struct getfeed_feed *create_feed(void);
void free_feed(struct getfeed_feed *feed);

// item bucket functions
void prepend_item(struct getfeed_item **head_item_ptr);
void free_item(struct getfeed_item *item);

bool prepend_category(struct getfeed_category **head_category_ptr);
void free_category(struct getfeed_category *category);

bool prepend_link(struct getfeed_link **head_link_ptr);
void free_link(struct getfeed_link *link);

bool prepend_person(struct getfeed_person **head_person_ptr);
void free_person(struct getfeed_person *person);

// date
time_t parse_date_rfc822(const struct string *value);
time_t parse_date_rfc3339(const char *src, size_t src_len);

// Element handlers

bool parse_namespace_element_start (struct xml_data *data, const char *name, const TidyAttr atts);
bool parse_namespace_element_end   (struct xml_data *data, const char *name);

#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
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
void parse_atom10_element_start (struct xml_data *data, const char *name, const TidyAttr atts);
void parse_atom10_element_end   (struct xml_data *data, const char *name);
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
enum rss20_position {
	RSS20_NONE = 0,
	RSS20_ITEM = 1,
	RSS20_TITLE = 2,
	RSS20_DESCRIPTION = 4,
	RSS20_LINK = 8,
	RSS20_PUBDATE = 16,
	RSS20_GUID = 32,
	RSS20_AUTHOR = 64,
	RSS20_SOURCE = 128,
	RSS20_CATEGORY = 256,
	RSS20_LASTBUILDDATE = 512,
	RSS20_COMMENTS = 1024,
	RSS20_LANGUAGE = 2048,
	RSS20_GENERATOR = 4096,
	RSS20_WEBMASTER = 8192,
	RSS20_MANAGINGEDITOR = 16384,
	RSS20_CHANNEL = 32768,
};
void parse_rss20_element_start (struct xml_data *data, const char *name, const TidyAttr atts);
void parse_rss20_element_end   (struct xml_data *data, const char *name);
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
enum dc_position {
	DC_NONE = 0,
	DC_TITLE = 1,
	DC_DESCRIPTION = 2,
	DC_CREATOR = 4,
	DC_SUBJECT = 8,
};
void parse_dc_element_start (struct xml_data *data, const char *name, const TidyAttr atts);
void parse_dc_element_end   (struct xml_data *data, const char *name);
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS10CONTENT
enum rss10content_position {
	RSS10CONTENT_NONE = 0,
	RSS10CONTENT_ENCODED = 1,
};
void parse_rss10content_element_start (struct xml_data *data, const char *name, const TidyAttr atts);
void parse_rss10content_element_end   (struct xml_data *data, const char *name);
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_YANDEX
enum yandex_position {
	YANDEX_NONE = 0,
	YANDEX_FULL_TEXT = 1,
	YANDEX_GENRE = 2,
	YANDEX_COMMENT_TEXT = 4,
	YANDEX_BIND_TO = 8,
};
void parse_yandex_element_start (struct xml_data *data, const char *name, const TidyAttr atts);
void parse_yandex_element_end   (struct xml_data *data, const char *name);
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM03
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
void parse_atom03_element_start (struct xml_data *data, const char *name, const TidyAttr atts);
void parse_atom03_element_end   (struct xml_data *data, const char *name);
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS11
enum rss11_position {
	RSS11_NONE = 0,
	RSS11_ITEM = 1,
	RSS11_TITLE = 2,
	RSS11_LINK = 4,
	RSS11_DESCRIPTION = 8,
	RSS11_IMAGE = 16,
	RSS11_URL = 32,
};
void parse_rss11_element_start (struct xml_data *data, const char *name, const TidyAttr atts);
void parse_rss11_element_end   (struct xml_data *data, const char *name);
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_JSONFEED
void json_dump_jsonfeed(cJSON *json, struct json_data *data);
#endif
#endif // GETFEED_H
