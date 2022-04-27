#ifndef UPDATE_FEED_H
#define UPDATE_FEED_H
#include <curl/curl.h>
#include "feedeater.h"

enum download_status {
	DOWNLOAD_SUCCEEDED,
	DOWNLOAD_CANCELED,
	DOWNLOAD_FAILED,
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
	struct string *etag_header;
	struct string *last_modified_header;
	struct getfeed_item *item;
};

struct curl_slist *create_list_of_headers(struct getfeed_feed *feed);
enum download_status download_feed(const char *url, struct getfeed_feed *feed, struct string *feedbuf);

bool initialize_feed(struct getfeed_feed *feed);
void free_feed(struct getfeed_feed *feed);

// See "parse_feed" directory for implementation.
bool parse_feed(const struct string *feed_buf, struct getfeed_feed *feed);

// See "insert_feed" directory for implementation.
bool insert_feed(const struct string *url, const struct getfeed_feed *feed);

// item bucket functions
bool prepend_item(struct getfeed_item **head_item_ptr);
void free_item(struct getfeed_item *item);

bool prepend_category(struct getfeed_category **head_category_ptr);
void free_category(struct getfeed_category *category);
struct string *generate_category_list_string(const struct getfeed_category *category);

bool prepend_link(struct getfeed_link **head_link_ptr);
void free_link(struct getfeed_link *link);
struct string *generate_link_list_string(const struct getfeed_link *link);

bool prepend_person(struct getfeed_person **head_person_ptr);
void free_person(struct getfeed_person *person);
struct string *generate_person_list_string(const struct getfeed_person *person);

struct string *generate_generator_string(const struct getfeed_generator *generator);

#endif // UPDATE_FEED_H
