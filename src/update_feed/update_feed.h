#ifndef UPDATE_FEED_H
#define UPDATE_FEED_H
#include <expat.h>
#include <yajl/yajl_parse.h>
#include "newsraft.h"

// Unknown type must have 0 value!
enum media_type {
	MEDIA_TYPE_UNKNOWN = 0,
	MEDIA_TYPE_XML,
	MEDIA_TYPE_JSON,
	MEDIA_TYPE_OTHER,
};

struct getfeed_item {
	struct string *guid;
	struct string *title;
	int8_t title_type;
	struct string *url;
	struct string *content;
	struct string *attachments;
	struct string *persons;
	struct string *extras;
	time_t publication_date; // Publication date in seconds since the Epoch (0 means unset).
	time_t update_date; // Update date in seconds since the Epoch (0 means unset).
	struct getfeed_item *next;
};

struct getfeed_feed {
	struct string *title;
	int8_t title_type;
	struct string *url;
	struct string *content;
	struct string *attachments;
	struct string *persons;
	struct string *extras;
	time_t download_date;
	time_t update_date;
	time_t time_to_live;
	struct string *http_header_etag;
	int64_t http_header_last_modified;
	int64_t http_header_expires;
	struct getfeed_item *item;
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
#endif // UPDATE_FEED_H
