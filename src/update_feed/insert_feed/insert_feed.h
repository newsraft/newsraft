#ifndef INSERT_FEED_H
#define INSERT_FEED_H
#include "update_feed/update_feed.h"

bool db_bind_text_struct(sqlite3_stmt *s, int8_t placeholder, struct getfeed_text *text_struct);

bool delete_excess_items(const struct string *feed_url, int64_t limit);

bool insert_feed_data(const struct string *feed_url, struct getfeed_feed *feed);
bool insert_item_data(const struct string *feed_url, struct getfeed_item *item);
#endif // INSERT_FEED_H
