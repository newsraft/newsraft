#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

struct feed_section {
	struct string *name;
	struct feed_line **feeds; // Array of pointers to related feed_line structs.
	size_t feeds_count; // Length of feeds array.
	size_t unread_count;
};

static struct feed_section *sections = NULL;
static size_t sections_count = 0;

static inline void
free_feed(struct feed_line *feed)
{
	if (feed == NULL) {
		return;
	}
	free_string(feed->name);
	free_string(feed->link);
	free(feed);
}

static bool
create_new_section(const struct string *section_name)
{
	size_t section_index = sections_count++;
	struct feed_section *temp = realloc(sections, sizeof(struct feed_section) * sections_count);
	if (temp == NULL) {
		fprintf(stderr, "Not enough memory for another section structure!\n");
		return false;
	}
	sections = temp;
	sections[section_index].name = crtss(section_name);
	if (sections[section_index].name == NULL) {
		fprintf(stderr, "Not enough memory for name string of the section!\n");
		return false;
	}
	sections[section_index].feeds = NULL;
	sections[section_index].feeds_count = 0;
	sections[section_index].unread_count = 0;
	INFO("Created \"%s\" section.", section_name->ptr);
	return true;
}

bool
create_global_section(void)
{
	return create_new_section(cfg.global_section_name);
}

// On success returns a pointer to the attached feed.
// On failure returns NULL.
static struct feed_line *
attach_feed_to_section(struct feed_line *feed, struct feed_section *section, bool do_free)
{
	INFO("Adding feed \"%s\" to section \"%s\".", feed->link->ptr, section->name->ptr);
	// We have to make sure that this feed is unique.
	for (size_t i = 0; i < section->feeds_count; ++i) {
		if (strcmp(section->feeds[i]->link->ptr, feed->link->ptr) == 0) {
			INFO("This feed is already in this section.");
			if (do_free == true) {
				free_feed(feed);
			}
			return section->feeds[i];
		}
	}
	size_t feed_index = (section->feeds_count)++;
	struct feed_line **temp = realloc(section->feeds, sizeof(struct feed_line *) * section->feeds_count);
	if (temp == NULL) {
		fprintf(stderr, "Not enough memory for new feed in section!\n");
		return NULL;
	}
	section->feeds = temp;
	section->feeds[feed_index] = feed;
	return section->feeds[feed_index];
}

static inline struct feed_line *
add_feed_to_global_section(struct feed_line *feed)
{
	return attach_feed_to_section(feed, &sections[0], true);
}

static inline struct feed_line *
add_feed_to_regular_section(struct feed_line *feed, struct feed_section *section)
{
	return attach_feed_to_section(feed, section, false);
}

bool
add_feed_to_section(struct feed_line *feed, const struct string *section_name)
{
	if (feed->link == NULL) {
		fprintf(stderr, "Encountered a NULL feed link while adding a new feed to the section!\n");
		return false;
	}
	struct feed_line *attached_feed = add_feed_to_global_section(feed);
	if (attached_feed == NULL) {
		return false;
	}
	if (strcmp(section_name->ptr, cfg.global_section_name->ptr) == 0) {
		// The section we add a feed to is global and we already added
		// a feed to the global section above. So exit innocently here.
		return true; // Not an error.
	}
	// Skip (i == 0) because first section is always the global one and
	// we already know that the section we add a feed to is not global.
	for (size_t i = 1; i < sections_count; ++i) {
		if (strcmp(section_name->ptr, sections[i].name->ptr) == 0) {
			if (add_feed_to_regular_section(attached_feed, &sections[i]) == NULL) {
				return false;
			}
			return true;
		}
	}
	if (create_new_section(section_name) == false) {
		return false;
	}
	if (add_feed_to_regular_section(attached_feed, &sections[sections_count - 1]) == NULL) {
		return false;
	}
	return true;
}

bool
obtain_feeds_of_section(const struct string *section_name, struct feed_line ***feeds_ptr, size_t *feeds_count_ptr)
{
	for (size_t i = 0; i < sections_count; ++i) {
		if (strcmp(section_name->ptr, sections[i].name->ptr) == 0) {
			*feeds_ptr = sections[i].feeds;
			*feeds_count_ptr = sections[i].feeds_count;
			return true;
		}
	}
	return false;
}

void
free_sections(void)
{
	for (size_t i = 0; i < sections[0].feeds_count; ++i) {
		free_feed(sections[0].feeds[i]);
	}
	for (size_t i = 0; i < sections_count; ++i) {
		free_string(sections[i].name);
		free(sections[i].feeds);
	}
	free(sections);
}
