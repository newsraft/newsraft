#include <string.h>
#include "feedeater.h"

struct feed_tag {
	char *name;
	struct string **urls;
	size_t urls_count;
};

static struct feed_tag *tags = NULL;
static size_t tags_count = 0;

static void
tag_init(char *tag_name, struct string *url)
{
	int tag_index = tags_count++;
	tags = realloc(tags, sizeof(struct feed_tag) * tags_count);
	tags[tag_index].name = malloc(sizeof(char) * (strlen(tag_name) + 1));
	tags[tag_index].urls = malloc(sizeof(struct string *));
	tags[tag_index].urls_count = 1;
	tags[tag_index].urls[0] = url;
	strcpy(tags[tag_index].name, tag_name);
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
	if (i == tags_count) tag_init(tag_name, url);
	debug_write(DBG_OK, "feed %s is tagged as %s\n", url->ptr, tag_name);
}

void
free_tags(void)
{
	if (tags == NULL) return;
	for (size_t i = 0; i < tags_count; ++i) {
		free(tags[i].name);
		free(tags[i].urls);
	}
	free(tags);
}
