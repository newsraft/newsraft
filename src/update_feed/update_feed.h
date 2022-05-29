#ifndef UPDATE_FEED_H
#define UPDATE_FEED_H
#include <expat.h>
#include <yajl/yajl_parse.h>
#include "newsraft.h"

enum download_status {
	DOWNLOAD_SUCCEEDED,
	DOWNLOAD_CANCELED,
	DOWNLOAD_FAILED,
};

// Unknown type must have 0 value!
enum {
	MEDIA_TYPE_UNKNOWN = 0,
	MEDIA_TYPE_XML,
	MEDIA_TYPE_JSON,
	MEDIA_TYPE_OTHER,
};

enum update_error {
	PARSE_OKAY = 0,
	PARSE_FAIL_NOT_ENOUGH_MEMORY,
};

enum xml_format_index {
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

struct getfeed_generator {
	struct string *name;
	struct string *version;
	struct string *url;
};

struct getfeed_text {
	struct string *value; // Text string.
	struct string *type;  // Standard MIME type of value.
};

struct getfeed_category {
	struct string *label;  // Provides a human-readable label for display.
	struct string *term;   // Identifies the category. If it's empty, then category is undefined.
	struct string *scheme; // Identifies the categorization scheme (mostly via an URI).
	struct getfeed_category *next;
};

struct getfeed_link {
	struct string *url;  // URL link to data.
	struct string *type; // Standard MIME type of data.
	size_t size;         // Size of data in bytes (0 means unset).
	size_t duration;     // Duration of data in seconds (0 means unset).
	struct getfeed_link *next;
};

struct getfeed_person {
	struct string *name;
	struct string *email;
	struct string *url;
	struct getfeed_person *next;
};

struct getfeed_picture {
	struct string *url;  // URL link to data.
	struct string *type; // Standard MIME type of data.
	size_t width;
	size_t height;
	size_t size;         // Size of data in bytes (0 means unset).
	struct getfeed_picture *next;
};

struct getfeed_item {
	struct string *guid;
	struct getfeed_text title;
	struct string *url;
	struct getfeed_text summary;
	struct getfeed_text content;
	struct getfeed_category *category;
	struct string *comments_url;
	struct getfeed_link *attachment;
	struct getfeed_person *author;
	struct getfeed_person *contributor;
	struct string_list *location;
	struct getfeed_picture *thumbnail;
	time_t pubdate; // Publication date in seconds since the Epoch (0 means unset).
	time_t upddate; // Update date in seconds since the Epoch (0 means unset).
	struct getfeed_item *next;
};

struct getfeed_feed {
	struct getfeed_text title;
	struct string *url;
	struct getfeed_text summary;
	struct getfeed_category *category;
	struct string *language;
	struct getfeed_generator generator;
	struct getfeed_text rights;
	struct getfeed_person *author;
	struct getfeed_person *editor;
	struct getfeed_person *webmaster;
	time_t update_date;
	time_t download_date;
	int64_t http_header_last_modified;
	int64_t http_header_expires;
	struct string *http_header_etag;
	struct getfeed_item *item;
};

struct stream_callback_data {
	int8_t media_type;
	XML_Parser xml_parser;
	yajl_handle json_parser;
	struct getfeed_feed feed;
	int8_t xml_format;
	intmax_t xml_pos[XML_FORMATS_COUNT];
	struct string *json_key;
	int8_t json_array_types[100];
	int8_t json_array_depth;
	struct string *value;
	uint64_t depth;
	int8_t error;
};

bool engage_xml_parser(struct stream_callback_data *data);
void free_xml_parser(struct stream_callback_data *data);

bool engage_json_parser(struct stream_callback_data *data);
void free_json_parser(struct stream_callback_data *data);

// See "download_feed" directory for implementation.
enum download_status download_feed(const char *url, struct stream_callback_data *data);

// See "parse_feed" directory for implementation.
bool parse_feed(const struct string *feed_buf, struct getfeed_feed *feed);

// See "insert_feed" directory for implementation.
bool insert_feed(const struct string *url, struct getfeed_feed *feed);

// item bucket functions
bool prepend_item(struct getfeed_item **head_item_ptr);
void free_item(struct getfeed_item *item);

bool prepend_category(struct getfeed_category **head_category_ptr);
void reverse_category_list(struct getfeed_category **list);
void free_category(struct getfeed_category *category);
struct string *generate_category_list_string(const struct getfeed_category *category);

bool prepend_link(struct getfeed_link **head_link_ptr);
void reverse_link_list(struct getfeed_link **list);
void free_link(struct getfeed_link *link);
struct string *generate_link_list_string(const struct getfeed_link *link);

bool prepend_person(struct getfeed_person **head_person_ptr);
void reverse_person_list(struct getfeed_person **list);
void free_person(struct getfeed_person *person);
struct string *generate_person_list_string(const struct getfeed_person *person);

bool prepend_empty_picture(struct getfeed_picture **head_picture_ptr);
void reverse_picture_list(struct getfeed_picture **head_picture_ptr);
void free_picture(struct getfeed_picture *picture);
struct string *generate_picture_list_string(const struct getfeed_picture *picture);

struct string *generate_generator_string(const struct getfeed_generator *generator);
#endif // UPDATE_FEED_H
