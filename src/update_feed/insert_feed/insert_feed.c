#include "update_feed/insert_feed/insert_feed.h"

bool
insert_feed(const struct feed_entry *feed, struct getfeed_feed *feed_data)
{
	if (insert_feed_data(feed->link, feed_data) == false) {
		FAIL("Failed to insert feed data!");
		return false;
	}
	for (struct getfeed_item *item = feed_data->item; item != NULL; item = item->next) {
		if (insert_item_data(feed->link, item) == false) {
			FAIL("Failed to insert item data!");
			return false;
		}
	}
	if (feed->capacity_limit > 0 && delete_excess_items(feed->link, feed->capacity_limit) == false) {
		WARN("Failed to delete excess items!");
	}
	return true;
}
