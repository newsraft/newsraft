#include "update_feed/update_feed.h"

// On success returns true.
// On memory shortage returns false.
bool
grow_meat_on_bones_of_the_feed(struct getfeed_feed *feed)
{
	if (time(&feed->download_date) == (time_t) -1)   { goto undo0; }
	if ((feed->title.value       = crtes()) == NULL) { goto undo0; }
	if ((feed->title.type        = crtes()) == NULL) { goto undo1; }
	if ((feed->url               = crtes()) == NULL) { goto undo2; }
	if ((feed->summary.value     = crtes()) == NULL) { goto undo3; }
	if ((feed->summary.type      = crtes()) == NULL) { goto undo4; }
	if ((feed->language          = crtes()) == NULL) { goto undo5; }
	if ((feed->generator.name    = crtes()) == NULL) { goto undo6; }
	if ((feed->generator.version = crtes()) == NULL) { goto undo7; }
	if ((feed->generator.url     = crtes()) == NULL) { goto undo8; }
	if ((feed->rights.value      = crtes()) == NULL) { goto undo9; }
	if ((feed->rights.type       = crtes()) == NULL) { goto undoA; }
	feed->category = NULL;
	feed->author = NULL;
	feed->editor = NULL;
	feed->webmaster = NULL;
	feed->update_date = 0;
	feed->item = NULL;
	return true;
undoA:
	free_string(feed->rights.value);
undo9:
	free_string(feed->generator.url);
undo8:
	free_string(feed->generator.version);
undo7:
	free_string(feed->generator.name);
undo6:
	free_string(feed->language);
undo5:
	free_string(feed->summary.type);
undo4:
	free_string(feed->summary.value);
undo3:
	free_string(feed->url);
undo2:
	free_string(feed->title.type);
undo1:
	free_string(feed->title.value);
undo0:
	return false;
}

void
free_feed(struct getfeed_feed *feed)
{
	free_string(feed->title.value);
	free_string(feed->title.type);
	free_string(feed->url);
	free_string(feed->summary.value);
	free_string(feed->summary.type);
	free_string(feed->language);
	free_string(feed->generator.name);
	free_string(feed->generator.version);
	free_string(feed->generator.url);
	free_string(feed->rights.value);
	free_string(feed->rights.type);
	free_category(feed->category);
	free_person(feed->author);
	free_person(feed->editor);
	free_person(feed->webmaster);
	free_string(feed->etag_header);
	free_string(feed->last_modified_header);
	free_item(feed->item);
}
