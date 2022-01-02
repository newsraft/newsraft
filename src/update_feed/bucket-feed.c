#include "feedeater.h"
#include "update_feed/update_feed.h"

// On success returns true.
// On memory shortage returns false.
bool
initialize_feed_bucket(struct feed_bucket *feed)
{
	if ((feed->title = create_empty_string()) == NULL) {
		goto undo0;
	}
	if ((feed->link = create_empty_string()) == NULL) {
		goto undo1;
	}
	if ((feed->summary = create_empty_string()) == NULL) {
		goto undo2;
	}
	if ((feed->summary_type = create_empty_string()) == NULL) {
		goto undo3;
	}
	if ((feed->categories = create_empty_string()) == NULL) {
		goto undo4;
	}
	if ((feed->language = create_empty_string()) == NULL) {
		goto undo5;
	}
	if ((feed->generator = create_empty_string()) == NULL) {
		goto undo6;
	}
	if ((feed->rights = create_empty_string()) == NULL) {
		goto undo7;
	}
	return true;
undo7:
	free_string(feed->generator);
undo6:
	free_string(feed->language);
undo5:
	free_string(feed->categories);
undo4:
	free_string(feed->summary_type);
undo3:
	free_string(feed->summary);
undo2:
	free_string(feed->link);
undo1:
	free_string(feed->title);
undo0:
	return false;
}

void
free_feed_bucket(const struct feed_bucket *feed)
{
	free_string(feed->title);
	free_string(feed->link);
	free_string(feed->summary);
	free_string(feed->summary_type);
	free_string(feed->categories);
	free_string(feed->language);
	free_string(feed->generator);
	free_string(feed->rights);
}
