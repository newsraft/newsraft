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

bool
create_global_section(void)
{
	sections = malloc(sizeof(struct feed_section));
	if (sections == NULL) {
		return false;
	}
	sections_count = 1;
	sections[0].name = crtas(cfg.global_section_name, strlen(cfg.global_section_name));
	if (sections[0].name == NULL) {
		free(sections);
		return false;
	}
	sections[0].feeds = NULL;
	sections[0].feeds_count = 0;
	sections[0].unread_count = 0;
	return true;
}

static bool
create_new_section(const struct string *section_name)
{
	size_t section_index = sections_count++;
	struct feed_section *temp = realloc(sections, sizeof(struct feed_section) * sections_count);
	if (temp == NULL) {
		FAIL("Not enough memory for another section structure!");
		return false;
	}
	sections = temp;
	sections[section_index].name = crtss(section_name);
	if (sections[section_index].name == NULL) {
		FAIL("Not enough memory for name string of the section!");
		return false;
	}
	sections[section_index].feeds = NULL;
	sections[section_index].feeds_count = 0;
	sections[section_index].unread_count = 0;
	INFO("Created \"%s\" section.", section_name->ptr);
	return true;
}

static bool
add_feed_to_certain_section(const struct feed_line *feed, struct feed_section *section)
{
	INFO("Adding feed \"%s\" to global section.", feed->link->ptr);
	size_t feed_index = (sections[0].feeds_count)++;
	struct feed_line **temp = realloc(sections[0].feeds, sizeof(struct feed_line *) * sections[0].feeds_count);
	if (temp == NULL) {
		return false;
	}
	sections[0].feeds = temp;
	sections[0].feeds[feed_index] = malloc(sizeof(struct feed_line));
	if (sections[0].feeds[feed_index] == NULL) {
		return false;
	}
	sections[0].feeds[feed_index]->name = feed->name;
	sections[0].feeds[feed_index]->link = feed->link;
	sections[0].feeds[feed_index]->unread_count = feed->unread_count;

	if ((section->name->len != 0) && (strcmp(section->name->ptr, cfg.global_section_name) != 0)) {
		INFO("Adding feed \"%s\" to section named \"%s\".", feed->link->ptr, section->name->ptr);
		size_t old_feed_index = feed_index;
		feed_index = (section->feeds_count)++;
		temp = realloc(section->feeds, sizeof(struct feed_line *) * section->feeds_count);
		if (temp == NULL) {
			return false;
		}
		section->feeds = temp;
		section->feeds[feed_index] = sections[0].feeds[old_feed_index];
	}

	return true;
}

bool
add_feed_to_section(const struct feed_line *feed, const struct string *section_name)
{
	for (size_t i = 0; i < sections_count; ++i) {
		if (strcmp(section_name->ptr, sections[i].name->ptr) == 0) {
			if (add_feed_to_certain_section(feed, &sections[i]) == false) {
				return false;
			}
			return true;
		}
	}
	if (create_new_section(section_name) == false) {
		return false;
	}
	if (add_feed_to_certain_section(feed, &sections[sections_count - 1]) == false) {
		return false;
	}
	return true;
}

bool
obtain_feeds_of_section(const char *section_name, struct feed_line ***feeds_ptr, size_t *feeds_count_ptr)
{
	for (size_t i = 0; i < sections_count; ++i) {
		if (strcmp(section_name, sections[i].name->ptr) == 0) {
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
		free_string(sections[0].feeds[i]->name);
		free_string(sections[0].feeds[i]->link);
		free(sections[0].feeds[i]);
	}
	for (size_t i = 0; i < sections_count; ++i) {
		free_string(sections[i].name);
		free(sections[i].feeds);
	}
	free(sections);
}
