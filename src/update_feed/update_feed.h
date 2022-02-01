#ifndef UPDATE_FEED_H
#define UPDATE_FEED_H
#include "feedeater.h"

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
	time_t update_time;
	time_t download_time;
	struct getfeed_item *item;
};

struct string *download_feed(const char *url);

// See "parse_feed" directory for implementation.
struct getfeed_feed *parse_feed(const struct string *feed_buf);
void free_feed(struct getfeed_feed *feed);

void delete_excess_items(const struct string *feed_url);

bool db_bind_text_struct(sqlite3_stmt *s, intmax_t placeholder, const struct getfeed_text *text_struct);

struct string *generate_link_list_string(const struct getfeed_link *link);
struct string *generate_person_list_string(const struct getfeed_person *person);
struct string *generate_generator_string(const struct getfeed_generator *generator);
struct string *generate_category_list_string(const struct getfeed_category *category);

void insert_item(const struct string *feed_url, const struct getfeed_item *item);
bool insert_feed(const struct string *feed_url, const struct getfeed_feed *feed);

#endif // UPDATE_FEED_H
