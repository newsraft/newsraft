#include "feedeater.h"
#include "update_feed/update_feed.h"

// On success returns true.
// On memory shortage returns false.
bool
initialize_item_bucket(struct item_bucket *item)
{
	if ((item->guid = create_empty_string()) == NULL) {
		goto undo0;
	}
	if ((item->title = create_empty_string()) == NULL) {
		goto undo1;
	}
	if ((item->title_type = create_empty_string()) == NULL) {
		goto undo2;
	}
	if ((item->url = create_empty_string()) == NULL) {
		goto undo3;
	}
	if ((item->categories = create_empty_string()) == NULL) {
		goto undo4;
	}
	if ((item->comments = create_empty_string()) == NULL) {
		goto undo5;
	}
	if ((item->summary = create_empty_string()) == NULL) {
		goto undo6;
	}
	if ((item->summary_type = create_empty_string()) == NULL) {
		goto undo7;
	}
	if ((item->content = create_empty_string()) == NULL) {
		goto undo8;
	}
	if ((item->content_type = create_empty_string()) == NULL) {
		goto undo9;
	}
	initialize_link_list(&item->enclosures);
	initialize_person_list(&item->authors);
	item->pubdate = 0;
	item->upddate = 0;
	return true;
undo9:
	free_string(item->content);
undo8:
	free_string(item->summary_type);
undo7:
	free_string(item->summary);
undo6:
	free_string(item->comments);
undo5:
	free_string(item->categories);
undo4:
	free_string(item->url);
undo3:
	free_string(item->title_type);
undo2:
	free_string(item->title);
undo1:
	free_string(item->guid);
undo0:
	return false;
}

void
empty_item_bucket(struct item_bucket *item)
{
	empty_string(item->guid);
	empty_string(item->title);
	empty_string(item->title_type);
	empty_string(item->url);
	empty_string(item->categories);
	empty_string(item->comments);
	empty_string(item->summary);
	empty_string(item->summary_type);
	empty_string(item->content);
	empty_string(item->content_type);
	empty_link_list(&item->enclosures);
	empty_person_list(&item->authors);
	item->pubdate = 0;
	item->upddate = 0;
}

void
free_item_bucket(const struct item_bucket *item)
{
	free_string(item->guid);
	free_string(item->title);
	free_string(item->title_type);
	free_string(item->url);
	free_string(item->categories);
	free_string(item->comments);
	free_string(item->summary);
	free_string(item->summary_type);
	free_string(item->content);
	free_string(item->content_type);
	free_link_list(&item->enclosures);
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
