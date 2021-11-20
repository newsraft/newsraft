#include <stdlib.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

struct item_bucket *
create_item_bucket(void)
{
	struct item_bucket *bucket = malloc(sizeof(struct item_bucket));
	if (bucket == NULL) goto create_item_bucket_undo1;
	if ((bucket->guid = create_empty_string()) == NULL) goto create_item_bucket_undo2;
	if ((bucket->title = create_empty_string()) == NULL) goto create_item_bucket_undo3;
	if ((bucket->url = create_empty_string()) == NULL) goto create_item_bucket_undo4;
	if ((bucket->categories = create_empty_string()) == NULL) goto create_item_bucket_undo5;
	if ((bucket->comments = create_empty_string()) == NULL) goto create_item_bucket_undo6;
	if ((bucket->content = create_empty_string()) == NULL) goto create_item_bucket_undo7;
	bucket->links = NULL;
	bucket->links_count = 0;
	bucket->authors = NULL;
	bucket->authors_count = 0;
	bucket->pubdate = 0;
	bucket->upddate = 0;
	return bucket; // success
create_item_bucket_undo7:
	free_string(bucket->comments);
create_item_bucket_undo6:
	free_string(bucket->categories);
create_item_bucket_undo5:
	free_string(bucket->url);
create_item_bucket_undo4:
	free_string(bucket->title);
create_item_bucket_undo3:
	free_string(bucket->guid);
create_item_bucket_undo2:
	free(bucket);
create_item_bucket_undo1:
	return NULL; // failure
}

void
drop_item_bucket(struct item_bucket *bucket)
{
	empty_string(bucket->guid);
	empty_string(bucket->title);
	empty_string(bucket->url);
	empty_string(bucket->categories);
	empty_string(bucket->comments);
	empty_string(bucket->content);
	bucket->pubdate = 0;
	bucket->upddate = 0;

	for (size_t i = 0; i < bucket->links_count; ++i) {
		free_string(bucket->links[i].url);
		free_string(bucket->links[i].type);
	}
	bucket->links_count = 0;

	for (size_t i = 0; i < bucket->authors_count; ++i) {
		free_string(bucket->authors[i].name);
		free_string(bucket->authors[i].link);
		free_string(bucket->authors[i].email);
	}
	bucket->authors_count = 0;
}

void
free_item_bucket(struct item_bucket *bucket)
{
	free_string(bucket->guid);
	free_string(bucket->title);
	free_string(bucket->url);
	free_string(bucket->categories);
	free_string(bucket->comments);
	free_string(bucket->content);

	for (size_t i = 0; i < bucket->links_count; ++i) {
		free_string(bucket->links[i].url);
		free_string(bucket->links[i].type);
	}
	free(bucket->links);

	for (size_t i = 0; i < bucket->authors_count; ++i) {
		free_string(bucket->authors[i].name);
		free_string(bucket->authors[i].link);
		free_string(bucket->authors[i].email);
	}
	free(bucket->authors);

	free(bucket);
}

void
add_category_to_item_bucket(const struct item_bucket *bucket, const char *category, size_t category_len)
{
	if ((category == NULL) || (category_len == 0)) {
		return;
	}
	if (bucket->categories->len != 0) {
		cat_string_array(bucket->categories, ", ", 2);
	}
	cat_string_array(bucket->categories, category, category_len);
}
