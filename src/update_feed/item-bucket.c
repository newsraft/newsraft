#include <stdlib.h>
#include <string.h>
#include "feedeater.h"
#include "update_feed/update_feed.h"

// On success returns a pointer to struct item_bucket.
// On failure returns NULL.
struct item_bucket *
create_item_bucket(void)
{
	struct item_bucket *bucket = malloc(sizeof(struct item_bucket));
	if (bucket == NULL) {
		goto undo1;
	}
	if ((bucket->guid = create_empty_string()) == NULL) {
		goto undo2;
	}
	if ((bucket->title = create_empty_string()) == NULL) {
		goto undo3;
	}
	if ((bucket->title_type = create_empty_string()) == NULL) {
		goto undo4;
	}
	if ((bucket->url = create_empty_string()) == NULL) {
		goto undo5;
	}
	if ((bucket->categories = create_empty_string()) == NULL) {
		goto undo6;
	}
	if ((bucket->comments = create_empty_string()) == NULL) {
		goto undo7;
	}
	if ((bucket->summary = create_empty_string()) == NULL) {
		goto undo8;
	}
	if ((bucket->summary_type = create_empty_string()) == NULL) {
		goto undo9;
	}
	if ((bucket->content = create_empty_string()) == NULL) {
		goto undo10;
	}
	if ((bucket->content_type = create_empty_string()) == NULL) {
		goto undo11;
	}
	bucket->enclosures = NULL;
	bucket->enclosures_len = 0;
	bucket->enclosures_lim = 0;
	bucket->authors = NULL;
	bucket->authors_len = 0;
	bucket->authors_lim = 0;
	bucket->pubdate = 0;
	bucket->upddate = 0;
	return bucket;
undo11:
	free_string(bucket->content);
undo10:
	free_string(bucket->summary_type);
undo9:
	free_string(bucket->summary);
undo8:
	free_string(bucket->comments);
undo7:
	free_string(bucket->categories);
undo6:
	free_string(bucket->url);
undo5:
	free_string(bucket->title_type);
undo4:
	free_string(bucket->title);
undo3:
	free_string(bucket->guid);
undo2:
	free(bucket);
undo1:
	return NULL;
}

void
empty_item_bucket(struct item_bucket *bucket)
{
	empty_string(bucket->guid);
	empty_string(bucket->title);
	empty_string(bucket->title_type);
	empty_string(bucket->url);
	empty_string(bucket->categories);
	empty_string(bucket->comments);
	empty_string(bucket->summary);
	empty_string(bucket->summary_type);
	empty_string(bucket->content);
	empty_string(bucket->content_type);
	bucket->pubdate = 0;
	bucket->upddate = 0;
	bucket->enclosures_len = 0;
	bucket->authors_len = 0;
}

void
free_item_bucket(struct item_bucket *bucket)
{
	free_string(bucket->guid);
	free_string(bucket->title);
	free_string(bucket->title_type);
	free_string(bucket->url);
	free_string(bucket->categories);
	free_string(bucket->comments);
	free_string(bucket->summary);
	free_string(bucket->summary_type);
	free_string(bucket->content);
	free_string(bucket->content_type);

	for (size_t i = 0; i < bucket->enclosures_lim; ++i) {
		free_string(bucket->enclosures[i].url);
		free_string(bucket->enclosures[i].type);
	}
	free(bucket->enclosures);

	for (size_t i = 0; i < bucket->authors_lim; ++i) {
		free_string(bucket->authors[i].name);
		free_string(bucket->authors[i].link);
		free_string(bucket->authors[i].email);
	}
	free(bucket->authors);

	free(bucket);
}

// On success returns 0.
// On failure returns non-zero.
int
add_category_to_item_bucket(const struct item_bucket *bucket, const char *value, size_t value_len)
{
	if (value_len == 0) {
		return 0; // Not an error, category is just empty.
	}
	if (bucket->categories->len != 0) {
		if (catas(bucket->categories, ", ", 2) != 0) {
			return 1;
		}
	}
	if (catas(bucket->categories, value, value_len) != 0) {
		return 1;
	}
	return 0;
}

// On success returns 0.
// On failure returns non-zero.
int
expand_enclosures_of_item_bucket_by_one_element(struct item_bucket *bucket)
{
	if (bucket->enclosures_len == bucket->enclosures_lim) {
		struct link *temp = realloc(bucket->enclosures, sizeof(struct link) * (bucket->enclosures_lim + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for item enclosure.");
			return 1;
		}
		bucket->enclosures = temp;
		++(bucket->enclosures_lim);
		if ((bucket->enclosures[bucket->enclosures_len].url = create_empty_string()) == NULL) {
			FAIL("Not enough memory for item enclosure URL string.");
			bucket->enclosures[bucket->enclosures_len].type = NULL;
			return 1;
		}
		if ((bucket->enclosures[bucket->enclosures_len].type = create_empty_string()) == NULL) {
			FAIL("Not enough memory for item enclosure data type string.");
			return 1;
		}
	} else {
		empty_string(bucket->enclosures[bucket->enclosures_len].url);
		empty_string(bucket->enclosures[bucket->enclosures_len].type);
	}
	bucket->enclosures[bucket->enclosures_len].size = 0;
	bucket->enclosures[bucket->enclosures_len].duration = 0;
	++(bucket->enclosures_len);
	return 0;
}

int
add_url_to_last_enclosure_of_item_bucket(struct item_bucket *bucket, const char *value, size_t value_len)
{
	return cpyas(bucket->enclosures[bucket->enclosures_len - 1].url, value, value_len);
}

int
add_type_to_last_enclosure_of_item_bucket(struct item_bucket *bucket, const char *value, size_t value_len)
{
	return cpyas(bucket->enclosures[bucket->enclosures_len - 1].type, value, value_len);
}

// On success returns 0.
// On failure returns non-zero.
int
add_size_to_last_enclosure_of_item_bucket(struct item_bucket *bucket, const char *value)
{
	int size;
	if (sscanf(value, "%d", &size) == 1) {
		bucket->enclosures[bucket->enclosures_len - 1].size = size;
		return 0;
	}
	return 1;
}

// On success returns 0.
// On failure returns non-zero.
int
expand_authors_of_item_bucket_by_one_element(struct item_bucket *bucket)
{
	if (bucket->authors_len == bucket->authors_lim) {
		struct author *temp = realloc(bucket->authors, sizeof(struct author) * (bucket->authors_lim + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for item author.");
			return 1;
		}
		bucket->authors = temp;
		++(bucket->authors_lim);
		if ((bucket->authors[bucket->authors_len].name = create_empty_string()) == NULL) {
			FAIL("Not enough memory for item author name string.");
			bucket->authors[bucket->authors_len].email = NULL;
			bucket->authors[bucket->authors_len].link = NULL;
			return 1;
		}
		if ((bucket->authors[bucket->authors_len].email = create_empty_string()) == NULL) {
			FAIL("Not enough memory for item author email string.");
			bucket->authors[bucket->authors_len].link = NULL;
			return 1;
		}
		if ((bucket->authors[bucket->authors_len].link = create_empty_string()) == NULL) {
			FAIL("Not enough memory for item author link string.");
			return 1;
		}
	} else {
		empty_string(bucket->authors[bucket->authors_len].name);
		empty_string(bucket->authors[bucket->authors_len].email);
		empty_string(bucket->authors[bucket->authors_len].link);
	}
	++(bucket->authors_len);
	return 0;
}

int
add_name_to_last_author_of_item_bucket(struct item_bucket *bucket, const struct string *value)
{
	return cpyss(bucket->authors[bucket->authors_len - 1].name, value);
}

int
add_email_to_last_author_of_item_bucket(struct item_bucket *bucket, const struct string *value)
{
	return cpyss(bucket->authors[bucket->authors_len - 1].email, value);
}

int
add_link_to_last_author_of_item_bucket(struct item_bucket *bucket, const struct string *value)
{
	return cpyss(bucket->authors[bucket->authors_len - 1].link, value);
}
