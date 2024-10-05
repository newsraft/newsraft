#ifndef INSERT_FEED_H
#define INSERT_FEED_H
#include "newsraft.h"

bool delete_excess_items(struct feed_entry *feed, int64_t limit);
bool insert_feed_data(const struct feed_entry *feed_entry, struct getfeed_feed *feed);
bool insert_item_data(struct feed_entry *feed, struct getfeed_item *item);
#endif // INSERT_FEED_H
