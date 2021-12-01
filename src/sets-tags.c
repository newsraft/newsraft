#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

static struct feed_tag *tags = NULL;
static size_t tags_count = 0;

static int
init_tag_with_first_url(const char *tag_name, const struct string *url)
{
	size_t tag_index = tags_count++;
	tags = realloc(tags, sizeof(struct feed_tag) * tags_count);
	if (tags == NULL) {
		return 1; // failure
	}
	tags[tag_index].name = malloc(sizeof(char) * (strlen(tag_name) + 1));
	if (tags[tag_index].name == NULL) {
		/* initialize urls to avoid freeing of random memory in free_tags call */
		tags[tag_index].urls = NULL;
		return 1; // failure
	}
	tags[tag_index].urls = malloc(sizeof(struct string *));
	if (tags[tag_index].urls == NULL) {
		return 1; // failure
	}
	tags[tag_index].urls_count = 1;
	tags[tag_index].urls[0] = url;
	strcpy(tags[tag_index].name, tag_name);
	return 0; // success
}

static int
append_another_url_to_tag(size_t tag_index, const struct string *feed_url)
{
	size_t url_index = tags[tag_index].urls_count++;
	tags[tag_index].urls = realloc(tags[tag_index].urls, sizeof(struct string *) * tags[tag_index].urls_count);
	if (tags[tag_index].urls == NULL) {
		return 1; // failure
	}
	tags[tag_index].urls[url_index] = feed_url;
	return 0; // success
}

int
tag_feed(const char *tag_name, const struct string *url)
{
	size_t i;
	int error = 0;
	for (i = 0; i < tags_count; ++i) {
		if (strcmp(tag_name, tags[i].name) == 0) {
			if (append_another_url_to_tag(i, url) != 0) {
				error = 1;
			}
			break;
		}
	}
	if (i == tags_count) {
		// the feed is being tagged with unknown (new) tag
		if (init_tag_with_first_url(tag_name, url) != 0) {
			error = 1;
		}
	}
	if (error == 0) {
		INFO("Feed \"%s\" is tagged as \"%s\".", url->ptr, tag_name);
	} else {
		FAIL("Not enough memory for tagging the feed \"%s\" as \"%s\"!", url->ptr, tag_name);
	}
	return error;
}

void
free_tags(void)
{
	if (tags == NULL) {
		return;
	}
	for (size_t i = 0; i < tags_count; ++i) {
		free(tags[i].name);
		free(tags[i].urls);
	}
	free(tags);
}

const struct feed_tag *
get_tag_by_name(const char *name)
{
	for (size_t i = 0; i < tags_count; ++i) {
		if (strcmp(name, tags[i].name) == 0) {
			return &(tags[i]);
		}
	}
	return NULL;
}
