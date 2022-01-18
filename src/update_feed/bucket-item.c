#include "update_feed/update_feed.h"

// On success returns true.
// On memory shortage returns false.
bool
initialize_item_bucket(struct item_bucket *item)
{
	if ((item->guid          = crtes()) == NULL) { goto undo0; }
	if ((item->title.value   = crtes()) == NULL) { goto undo1; }
	if ((item->title.type    = crtes()) == NULL) { goto undo2; }
	if ((item->url           = crtes()) == NULL) { goto undo3; }
	if ((item->summary.value = crtes()) == NULL) { goto undo4; }
	if ((item->summary.type  = crtes()) == NULL) { goto undo5; }
	if ((item->content.value = crtes()) == NULL) { goto undo6; }
	if ((item->content.type  = crtes()) == NULL) { goto undo7; }
	if ((item->categories    = crtes()) == NULL) { goto undo8; }
	if ((item->comments_url  = crtes()) == NULL) { goto undo9; }
	initialize_link_list(&item->attachments);
	initialize_person_list(&item->authors);
	item->pubdate = 0;
	item->upddate = 0;
	return true;
undo9:
	free_string(item->categories);
undo8:
	free_string(item->content.type);
undo7:
	free_string(item->content.value);
undo6:
	free_string(item->summary.type);
undo5:
	free_string(item->summary.value);
undo4:
	free_string(item->url);
undo3:
	free_string(item->title.type);
undo2:
	free_string(item->title.value);
undo1:
	free_string(item->guid);
undo0:
	return false;
}

void
empty_item_bucket(struct item_bucket *item)
{
	empty_string(item->guid);
	empty_string(item->title.value);
	empty_string(item->title.type);
	empty_string(item->url);
	empty_string(item->summary.value);
	empty_string(item->summary.type);
	empty_string(item->content.value);
	empty_string(item->content.type);
	empty_string(item->categories);
	empty_string(item->comments_url);
	empty_link_list(&item->attachments);
	empty_person_list(&item->authors);
	item->pubdate = 0;
	item->upddate = 0;
}

void
free_item_bucket(const struct item_bucket *item)
{
	free_string(item->guid);
	free_string(item->title.value);
	free_string(item->title.type);
	free_string(item->url);
	free_string(item->summary.value);
	free_string(item->summary.type);
	free_string(item->content.value);
	free_string(item->content.type);
	free_string(item->categories);
	free_string(item->comments_url);
	free_link_list(&item->attachments);
	free_person_list(&item->authors);
}

// On success returns true.
// On memory shortage returns false.
bool
add_category_to_item_bucket(const struct item_bucket *item, const char *value, size_t value_len)
{
	if (value_len == 0) {
		return true; // Not an error, category is just empty.
	}
	if (item->categories->len != 0) {
		if (catas(item->categories, ", ", 2) == false) {
			return false;
		}
	}
	if (catas(item->categories, value, value_len) == false) {
		return false;
	}
	return true;
}
