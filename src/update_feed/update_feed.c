#include <stdbool.h>
#include <getfeed.h>

void insert_item(const struct string *feed_url, const struct item_bucket *item);
bool insert_feed(const struct string *feed_url, const struct feed_bucket *feed);

bool
update_feed(const struct string *url)
{
	struct getfeed_feed *feed = getfeed(url->ptr);
	if (feed == NULL) {
		return false;
	}
	struct item_bucket *item = feed->item;
	while (item != NULL) {
		insert_item(url, item);
		item = item->next;
	}
	insert_feed(url, feed->data);
	freefeed(feed);
	return true;
}
