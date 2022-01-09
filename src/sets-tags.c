#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

// On success returns 0.
// On failure returns non-zero.
static inline int
init_tag_with_first_url(struct feed_tag **head_tag_ptr, const char *tag_name, size_t tag_name_len, const struct string *url)
{
	*head_tag_ptr = malloc(sizeof(struct feed_tag));
	if (*head_tag_ptr == NULL) {
		return 1;
	}
	(*head_tag_ptr)->next_tag = NULL;
	(*head_tag_ptr)->name = malloc(sizeof(char) * (tag_name_len + 1));
	if ((*head_tag_ptr)->name == NULL) {
		// Initialize pointer to avoid freeing of random memory in free_tags call.
		(*head_tag_ptr)->urls = NULL;
		return 1;
	}
	(*head_tag_ptr)->urls = malloc(sizeof(struct string *));
	if ((*head_tag_ptr)->urls == NULL) {
		return 1;
	}
	(*head_tag_ptr)->urls[0] = url;
	(*head_tag_ptr)->urls_count = 1;
	strcpy((*head_tag_ptr)->name, tag_name);
	return 0;
}

// On success returns 0.
// On failure returns non-zero.
static inline int
append_another_url_to_tag(struct feed_tag *tag, const struct string *feed_url)
{
	const struct string **temp = realloc(tag->urls, sizeof(struct string *) * (tag->urls_count + 1));
	if (temp == NULL) {
		return 1;
	}
	tag->urls = temp;
	tag->urls[tag->urls_count] = feed_url;
	++(tag->urls_count);
	return 0;
}

bool
tag_feed(struct feed_tag **head_tag_ptr, const char *tag_name, size_t tag_name_len, const struct string *url)
{
	int error = -1;
	struct feed_tag **tag_ptr = head_tag_ptr;
	while (*tag_ptr != NULL) {
		if (strcmp(tag_name, (*tag_ptr)->name) == 0) {
			if (append_another_url_to_tag(*tag_ptr, url) == 0) {
				error = 0;
			} else {
				error = 1;
			}
			break;
		}
		tag_ptr = &((*tag_ptr)->next_tag);
	}
	if (error == -1) {
		// the feed is being tagged with unknown (new) tag
		if (init_tag_with_first_url(tag_ptr, tag_name, tag_name_len, url) == 0) {
			error = 0;
		} else {
			error = 1;
		}
	}
	if (error == 0) {
		INFO("Feed \"%s\" is tagged as \"%s\".", url->ptr, tag_name);
		return true;
	} else {
		fprintf(stderr, "Not enough memory for tagging a feed!\n");
		return false;
	}
}

const struct feed_tag *
get_tag_by_name(const struct feed_tag *head_tag, const char *name)
{
	const struct feed_tag *tag = head_tag;
	while (tag != NULL) {
		if (strcmp(name, tag->name) == 0) {
			return tag;
		}
		tag = tag->next_tag;
	}
	return NULL;
}

void
free_tags(struct feed_tag *head_tag)
{
	INFO("Freeing tags.");
	struct feed_tag *tag = head_tag;
	struct feed_tag *tmp;
	while (tag != NULL) {
		tmp = tag;
		tag = tag->next_tag;
		free(tmp->name);
		free(tmp->urls);
		free(tmp);
	}
}
