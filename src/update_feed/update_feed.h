#ifndef UPDATE_FEED_H
#define UPDATE_FEED_H
#include <getfeed.h>
#include "feedeater.h"

struct string *download_feed(const char *url);

void delete_excess_items(const struct string *feed_url);

bool db_bind_text_struct(sqlite3_stmt *s, intmax_t placeholder, const struct getfeed_text *text_struct);

struct string *generate_link_list_string(const struct getfeed_link *link);
struct string *generate_person_list_string(const struct getfeed_person *person);
struct string *generate_generator_string(const struct getfeed_generator *generator);

void insert_item(const struct string *feed_url, const struct getfeed_item *item);
bool insert_feed(const struct string *feed_url, const struct getfeed_feed *feed);

#endif // UPDATE_FEED_H
