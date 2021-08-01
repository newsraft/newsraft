#include <string.h>
#include "feedeater.h"

static struct feed_tag *tags = NULL;
static size_t tags_count = 0;

static int
tag_init(char *tag_name, struct string *url)
{
	size_t tag_index = tags_count++;
	tags = realloc(tags, sizeof(struct feed_tag) * tags_count);
	if (tags == NULL) {
		return 1;
	}
	tags[tag_index].name = malloc(sizeof(char) * (strlen(tag_name) + 1));
	if (tags[tag_index].name == NULL) {
		return 1;
	}
	tags[tag_index].urls = malloc(sizeof(struct string *));
	if (tags[tag_index].urls == NULL) {
		free(tags[tag_index].name);
		return 1;
	}
	tags[tag_index].urls_count = 1;
	tags[tag_index].urls[0] = url;
	strcpy(tags[tag_index].name, tag_name);
	return 0;
}

void
tag_feed(char *tag_name, struct string *url)
{
	size_t url_index, i;
	for (i = 0; i < tags_count; ++i) {
		if (strcmp(tag_name, tags[i].name) == 0) {
			url_index = tags[i].urls_count++;
			tags[i].urls = realloc(tags[i].urls, sizeof(struct string *) * tags[i].urls_count);
			tags[i].urls[url_index] = url;
			break;
		}
	}
	if (i == tags_count) {
		if (tag_init(tag_name, url) != 0) {
			--tags_count;
			tags = realloc(tags, sizeof(struct feed_tag) * tags_count);
			debug_write(DBG_ERR, "failed to tag \"%s\" as \"%s\"\n", url->ptr, tag_name);
			return;
		}
	}
	debug_write(DBG_OK, "feed \"%s\" is tagged as \"%s\"\n", url->ptr, tag_name);
}

void
debug_tags_summary(void)
{
	if (tags_count == 0) {
		debug_write(DBG_INFO, "no feeds were tagged!\n");
		return;
	}
	debug_write(DBG_INFO, "tags summary:\n");
	for (size_t i = 0; i < tags_count; ++i) {
		debug_write(DBG_INFO, "  feeds related to tag \"%s\":\n", tags[i].name);
		for (size_t j = 0; j < tags[i].urls_count; ++j) {
			debug_write(DBG_INFO, "    %s\n", tags[i].urls[j]->ptr);
		}
	}
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

struct feed_tag *
get_tag_by_name(char *name)
{
	for (size_t i = 0; i < tags_count; ++i) {
		if (strcmp(name, tags[i].name) == 0) {
			return &(tags[i]);
		}
	}
	return NULL;
}
