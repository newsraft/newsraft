#include <string.h>
#include "feedeater.h"

struct feed_tag {
	char *name;
	struct string **urls;
	size_t urls_count;
};

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
			debug_write(DBG_WARN, "failed to tag feed %s as %s\n", url->ptr, tag_name);
			return;
		}
	}
	debug_write(DBG_OK, "feed %s is tagged as %s\n", url->ptr, tag_name);
}

struct set_statement *
create_set_statement(struct set_line *set)
{
	struct set_statement *st = malloc(sizeof(struct set_statement));
	if (st == NULL) {
		debug_write(DBG_ERR, "can't allocate memory for set_statement!\n");
		return NULL;
	}
	st->db_cmd = create_empty_string();
	st->urls = NULL;
	st->urls_count = 0;
	if (set->type == FEED_ENTRY) {
		cat_string_array(st->db_cmd, " feed = ?", 9);
		st->urls = malloc(sizeof(struct string *));
		if (st->urls == NULL) {
			free_string(st->db_cmd);
			free(st);
			debug_write(DBG_ERR, "can't allocate memory for set_statement url list!\n");
			return NULL;
		}
		st->urls_count = 1;
		st->urls[0] = set->data;
		return st;
	}
	char word[1000], c;
	size_t word_len, i = 0, old_urls_count;
	if (set->type == FILTER_ENTRY) {
		while (1) {
			word_len = 0;
			while (1) {
				c = set->data->ptr[i++];
				if (c == ' ' || c == '\t' || c == '\0') { word[word_len] = '\0'; break; }
				word[word_len++] = c;
			}
			if (word_len == 0) {
				if (c == '\0') {
					break;
				} else {
					continue;
				}
			}
			if (strcmp(word, "&") == 0) {
				cat_string_array(st->db_cmd, " AND", 4);
			} else if (strcmp(word, "|") == 0) {
				cat_string_array(st->db_cmd, " OR", 3);
			} else if (strcmp(word, "(") == 0) {
				cat_string_array(st->db_cmd, " (", 2);
			} else if (strcmp(word, ")") == 0) {
				cat_string_array(st->db_cmd, " )", 2);
			} else {
				for (size_t i = 0; i < tags_count; ++i) {
					if (strcmp(word, tags[i].name) == 0) {
						old_urls_count = st->urls_count;
						st->urls_count += tags[i].urls_count;
						st->urls = realloc(st->urls, sizeof(struct string *) * st->urls_count);
						for (size_t j = 0; j < tags[i].urls_count; ++j) {
							st->urls[old_urls_count++] = tags[i].urls[j];
						}
						cat_string_array(st->db_cmd, " (", 2);
						for (size_t j = 0; j < tags[i].urls_count; ++j) {
							cat_string_array(st->db_cmd, " feed = ?", 9);
							if (j + 1 != tags[i].urls_count) cat_string_array(st->db_cmd, " OR", 3);
						}
						cat_string_array(st->db_cmd, " )", 2);
						break;
					}
				}
			}
			if (c == '\0') break;
		}
		return st;
	}
	free_string(st->db_cmd);
	free(st);
	return NULL;
}

void
debug_tags_summary(void)
{
	debug_write(DBG_INFO, "tags summary:\n");
	for (size_t i = 0; i < tags_count; ++i) {
		debug_write(DBG_INFO, "feeds related to tag \"%s\":\n", tags[i].name);
		for (size_t j = 0; j < tags[i].urls_count; ++j) {
			debug_write(DBG_INFO, "%s\n", tags[i].urls[j]->ptr);
		}
	}
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
