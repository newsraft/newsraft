#include <string.h>
#include "feedeater.h"

const char *
get_value_of_attribute_key(const XML_Char **atts, const char *key)
{
	/* expat says that atts is name/value array (looks like [name1, value1, name2, value2, ...])
	 * so iterate with the step of 2 */
	for (size_t i = 0; atts[i] != NULL; i = i + 2) {
		if (strcmp(atts[i], key) == 0) {
			return atts[i + 1]; // success
		}
	}
	return NULL; // failure, didn't find an attribute with key name
}

int
init_item_bucket(struct item_bucket *bucket)
{
	if ((bucket->guid = create_empty_string()) == NULL) goto init_item_bucket_undo1;
	if ((bucket->title = create_empty_string()) == NULL) goto init_item_bucket_undo2;
	if ((bucket->url = create_empty_string()) == NULL) goto init_item_bucket_undo3;
	if ((bucket->categories = create_empty_string()) == NULL) goto init_item_bucket_undo4;
	if ((bucket->comments = create_empty_string()) == NULL) goto init_item_bucket_undo5;
	if ((bucket->content = create_empty_string()) == NULL) goto init_item_bucket_undo6;
	bucket->authors = NULL;
	bucket->authors_count = 0;
	bucket->pubdate = 0;
	bucket->upddate = 0;
	return 0; // success
init_item_bucket_undo6:
	free_string(bucket->comments);
init_item_bucket_undo5:
	free_string(bucket->categories);
init_item_bucket_undo4:
	free_string(bucket->url);
init_item_bucket_undo3:
	free_string(bucket->title);
init_item_bucket_undo2:
	free_string(bucket->guid);
init_item_bucket_undo1:
	return 1; // failure
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

	for (size_t i = 0; i < bucket->authors_count; ++i) {
		free_string(bucket->authors[i].name);
		free_string(bucket->authors[i].link);
		free_string(bucket->authors[i].email);
	}
	free(bucket->authors);
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
