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
	struct getfeed_item *next;
};

struct getfeed_feed {
	struct string *title;
	struct string *url;
	struct string *content;
	struct string *attachments;
	struct string *persons;
	struct string *extras;
	int64_t download_date;
	int64_t update_date;
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
	int8_t xml_format;
	struct string *text;
	uint8_t path[256];
	uint8_t depth;
	struct string decoy;
	struct string *emptying_target;
};

// See "threading.c" file for implementation.
bool initialize_update_threads(void);
void branch_update_feed_action_into_thread(void *(*action)(void *arg), struct feed_entry *feed);
void wait_for_all_threads_to_finish(void);
void terminate_update_threads(void);

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
