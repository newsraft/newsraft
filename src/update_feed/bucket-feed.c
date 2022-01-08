#include "update_feed/update_feed.h"

// On success returns true.
// On memory shortage returns false.
bool
initialize_feed_bucket(struct feed_bucket *feed)
{
	if ((feed->title.value     = crtes()) == NULL) { goto undo0; }
	if ((feed->title.type      = crtes()) == NULL) { goto undo1; }
	if ((feed->link            = crtes()) == NULL) { goto undo2; }
	if ((feed->summary.value   = crtes()) == NULL) { goto undo3; }
	if ((feed->summary.type    = crtes()) == NULL) { goto undo4; }
	if ((feed->categories      = crtes()) == NULL) { goto undo5; }
	if ((feed->language        = crtes()) == NULL) { goto undo6; }
	if ((feed->generator.value = crtes()) == NULL) { goto undo7; }
	if ((feed->generator.type  = crtes()) == NULL) { goto undo8; }
	if ((feed->rights.value    = crtes()) == NULL) { goto undo9; }
	if ((feed->rights.type     = crtes()) == NULL) { goto undoA; }
	return true;
undoA:
	free_string(feed->rights.value);
undo9:
	free_string(feed->generator.type);
undo8:
	free_string(feed->generator.value);
undo7:
	free_string(feed->language);
undo6:
	free_string(feed->categories);
undo5:
	free_string(feed->summary.type);
undo4:
	free_string(feed->summary.value);
undo3:
	free_string(feed->link);
undo2:
	free_string(feed->title.type);
undo1:
	free_string(feed->title.value);
undo0:
	return false;
}

void
free_feed_bucket(const struct feed_bucket *feed)
{
	free_string(feed->title.value);
	free_string(feed->title.type);
	free_string(feed->link);
	free_string(feed->summary.value);
	free_string(feed->summary.type);
	free_string(feed->categories);
	free_string(feed->language);
	free_string(feed->generator.value);
	free_string(feed->generator.type);
	free_string(feed->rights.value);
	free_string(feed->rights.type);
}
