#ifndef UPDATE_FEED_H
#define UPDATE_FEED_H
#include <expat.h>
#include <yajl/yajl_parse.h>
#include "newsraft.h"

typedef uint8_t download_status;
enum {
	DOWNLOAD_SUCCEEDED,
	DOWNLOAD_CANCELED,
	DOWNLOAD_FAILED,
};

struct getfeed_item {
	struct string *guid;
	struct string *title;
	struct string *url;
	struct string *content;
	struct string *attachments;
	struct string *persons;
	struct string *extras;
	int64_t publication_date; // Publication date in seconds since the Epoch (0 means unset).
	int64_t update_date; // Update date in seconds since the Epoch (0 means unset).
	bool guid_is_url;
	struct getfeed_item *next;
};

struct getfeed_feed {
	struct string *title;
	struct string *url;
	struct string *content;
	struct string *attachments;
	struct string *persons;
	struct string *extras;
	int64_t time_to_live;
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
	bool in_item;
	struct string *text;
	uint8_t path[256];
	uint8_t depth;
	struct string decoy;
	struct string *emptying_target;
};

download_status download_feed(struct feed_entry *feed, struct stream_callback_data *data);
download_status execute_feed(const struct string *cmd, struct stream_callback_data *data);

bool setup_xml_parser(struct stream_callback_data *data);
void free_xml_parser(struct stream_callback_data *data);

bool setup_json_parser(struct stream_callback_data *data);
void free_json_parser(struct stream_callback_data *data);

// See "insert_feed" directory for implementation.
bool insert_feed(struct feed_entry *feed, struct getfeed_feed *feed_data);

// item bucket functions
bool prepend_item(struct getfeed_item **head_item_ptr);
void free_item(struct getfeed_item *item);
#endif // UPDATE_FEED_H
