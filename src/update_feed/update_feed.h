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
enum media_type {
	MEDIA_TYPE_UNKNOWN = 0,
	MEDIA_TYPE_XML,
	MEDIA_TYPE_JSON,
	MEDIA_TYPE_OTHER,
};

enum person_involvement {
	PERSON_AUTHOR = 0,
	PERSON_CONTRIBUTOR,
	PERSON_EDITOR,
	PERSON_WEBMASTER,
	PERSON_ASSIGNEE,
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
	struct string *term;   // Identifies the category. If it's empty, then category is undefined.
	struct string *scheme; // Identifies the categorization scheme (mostly via an URI).
	struct string *label;  // Provides a human-readable label to display.
};

struct getfeed_link {
	struct string *url;  // URL link to data.
	struct string *type; // Standard MIME type of data.
	size_t size;         // Size of data in bytes (0 means unset).
	size_t duration;     // Duration of data in seconds (0 means unset).
};

struct getfeed_person {
	// What kind of participation did the person take in the publication (possible
	// values are presented in person_involvement enumeration.
	uint8_t involvement;
	struct string *name;
	struct string *email;
	struct string *url;
};

struct getfeed_source {
	struct string *url;
	struct string *title;
	bool url_is_set;
};

struct getfeed_picture {
	struct string *url;  // URL link to data.
	struct string *type; // Standard MIME type of data.
	size_t width;
	size_t height;
	size_t size;         // Size of data in bytes (0 means unset).
};

struct getfeed_temp {
	char buf[256];
	uint8_t buf_len;
	struct getfeed_person author;
	struct getfeed_link attachment;
	struct getfeed_category category;
	struct getfeed_source source;
	struct getfeed_picture picture;
};

struct getfeed_item {
	struct string *guid;
	struct getfeed_text title;
	struct string *url;
	struct getfeed_text summary;
	struct getfeed_text content;
	struct string *attachments;
	struct string *sources;
	struct string *authors;
	struct string *comments_url;
	struct string *locations;
	struct string *categories;
	struct string *language;
	struct getfeed_text rights;
	struct string *rating;
	struct string *pictures;
	time_t pubdate; // Publication date in seconds since the Epoch (0 means unset).
	time_t upddate; // Update date in seconds since the Epoch (0 means unset).
	struct getfeed_item *next;
};

struct getfeed_feed {
	struct string *guid;
	struct getfeed_text title;
	struct string *url;
	struct getfeed_text summary;
	struct string *authors;
	struct string *categories;
	struct string *language;
	struct getfeed_text rights;
	struct string *rating;
	struct string *pictures;
	struct getfeed_generator generator;
	time_t download_date;
	time_t update_date;
	time_t time_to_live;
	struct string *http_header_etag;
	int64_t http_header_last_modified;
	int64_t http_header_expires;
	struct getfeed_item *item;
	struct getfeed_temp temp;
};

struct stream_callback_data {
	int8_t media_type;
	XML_Parser xml_parser;
	yajl_handle json_parser;
	struct getfeed_feed feed;
	int8_t xml_format;
	struct string *text;
	uint8_t path[256];
	uint8_t depth;
};

enum download_status download_feed(const char *url, struct stream_callback_data *data);

bool engage_xml_parser(struct stream_callback_data *data);
void free_xml_parser(struct stream_callback_data *data);

bool engage_json_parser(struct stream_callback_data *data);
void free_json_parser(struct stream_callback_data *data);

// See "insert_feed" directory for implementation.
bool insert_feed(const struct string *url, struct getfeed_feed *feed);

// item bucket functions
bool prepend_item(struct getfeed_item **head_item_ptr);
void free_item(struct getfeed_item *item);

void empty_category(struct getfeed_temp *temp);
bool serialize_category(struct getfeed_temp *temp, struct string **str);

void empty_link(struct getfeed_temp *temp);
bool serialize_link(struct getfeed_temp *temp, struct string **str);

void empty_person(struct getfeed_temp *temp);
bool serialize_person(struct getfeed_temp *temp, struct string **str);

void empty_source(struct getfeed_temp *temp);
bool serialize_source(struct getfeed_temp *temp, struct string **str);

void empty_picture(struct getfeed_temp *temp);
bool serialize_picture(struct getfeed_temp *temp, struct string **str);

struct string *generate_generator_string(const struct getfeed_generator *generator);
#endif // UPDATE_FEED_H
