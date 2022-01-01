#include <stdlib.h>
#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

// On success returns a pointer to struct feed_bucket.
// On failure returns NULL.
struct feed_bucket *
create_feed_bucket(void)
{
	struct feed_bucket *feed = malloc(sizeof(struct feed_bucket));
	if (feed == NULL) {
		goto undo1;
	}
	if ((feed->title = create_empty_string()) == NULL) {
		goto undo2;
	}
	if ((feed->link = create_empty_string()) == NULL) {
		goto undo3;
	}
	if ((feed->summary = create_empty_string()) == NULL) {
		goto undo4;
	}
	if ((feed->summary_type = create_empty_string()) == NULL) {
		goto undo5;
	}
	if ((feed->categories = create_empty_string()) == NULL) {
		goto undo6;
	}
	if ((feed->language = create_empty_string()) == NULL) {
		goto undo7;
	}
	if ((feed->generator = create_empty_string()) == NULL) {
		goto undo8;
	}
	if ((feed->rights = create_empty_string()) == NULL) {
		goto undo9;
	}
	return feed;
undo9:
	free_string(feed->generator);
undo8:
	free_string(feed->language);
undo7:
	free_string(feed->categories);
undo6:
	free_string(feed->summary_type);
undo5:
	free_string(feed->summary);
undo4:
	free_string(feed->link);
undo3:
	free_string(feed->title);
undo2:
	free(feed);
undo1:
	return NULL;
}

void
free_feed_bucket(struct feed_bucket *feed)
{
	if (feed == NULL) {
		return;
	}
	free_string(feed->title);
	free_string(feed->link);
	free_string(feed->summary);
	free_string(feed->summary_type);
	free_string(feed->categories);
	free_string(feed->language);
	free_string(feed->generator);
	free_string(feed->rights);
	free(feed);
}
