#include <string.h>
#include "feedeater.h"

bool
add_another_url_to_trim_link_list(struct link_list *links, const char *url, size_t url_len)
{
	struct link *temp = realloc(links->list, sizeof(struct link) * (links->len + 1));
	if (temp == NULL) {
		return false;
	}
	links->list = temp;
	links->list[links->len].url = crtas(url, url_len);
	if (links->list[links->len].url == NULL) {
		return false;
	}
	links->list[links->len].type = NULL;
	links->list[links->len].size = NULL;
	links->list[links->len].duration = NULL;
	++(links->len);
	return true;
}

void
free_trim_link_list(const struct link_list *links)
{
	if (links->list == NULL) {
		return;
	}
	for (size_t i = 0; i < links->len; ++i) {
		free_string(links->list[i].url);
		free_string(links->list[i].type);
		free_string(links->list[i].size);
		free_string(links->list[i].duration);
	}
	free(links->list);
}

static inline bool
append_raw_link(struct link_list *links, sqlite3_stmt *res, enum item_column column)
{
	char *text = (char *)sqlite3_column_text(res, column);
	if (text == NULL) {
		return true; // It is not an error because this item simply does not have link set.
	}
	size_t text_len = strlen(text);
	if (text_len == 0) {
		return true; // It is not an error because this item simply does not have link set.
	}
	if (add_another_url_to_trim_link_list(links, text, text_len) == false) {
		return false;
	}
	return true;
}

enum attachment_field_index {
	ATTACHMENT_URL = 0,
	ATTACHMENT_TYPE,
	ATTACHMENT_SIZE,
	ATTACHMENT_DURATION,
};

static inline bool
append_attachments(struct link_list *links, sqlite3_stmt *res)
{
	const char *text = (const char *)sqlite3_column_text(res, ITEM_COLUMN_ATTACHMENTS);
	if (text == NULL) {
		return true; // It is not an error because this item simply does not have link set.
	}
	struct string *word = crtes();
	if (word == NULL) {
		return false;
	}
	int8_t field_num = ATTACHMENT_URL;
	struct string **target;
	const char *iter = text;
	while (*iter != '\0') {
		if ((*iter == ' ') || (*iter == '\n')) {
			if (word->len != 0) {
				target = NULL;
				if (field_num == ATTACHMENT_URL) {
					if (add_another_url_to_trim_link_list(links, word->ptr, word->len) == false) {
						goto error;
					}
				} else if (field_num == ATTACHMENT_TYPE) {
					target = &links->list[links->len - 1].type;
				} else if (field_num == ATTACHMENT_SIZE) {
					target = &links->list[links->len - 1].size;
				} else if (field_num == ATTACHMENT_DURATION) {
					target = &links->list[links->len - 1].duration;
				}
				if ((target != NULL) && (*target == NULL)) {
					*target = crtss(word);
					if (*target == NULL) {
						goto error;
					}
				}
			}
			if (*iter == '\n') {
				field_num = ATTACHMENT_TYPE;
			} else {
				++field_num;
			}
			empty_string(word);
		} else {
			if (catcs(word, *iter) == false) {
				goto error;
			}
		}
		++iter;
	}
	free_string(word);
	return true;
error:
	free_string(word);
	return false;
}

bool
populate_link_list_with_links_of_item(struct link_list *links, sqlite3_stmt *res)
{
	if (append_raw_link(links, res, ITEM_COLUMN_LINK) == false) {
		return false;
	}
	if (append_raw_link(links, res, ITEM_COLUMN_COMMENTS_URL) == false) {
		return false;
	}
	if (append_attachments(links, res) == false) {
		return false;
	}
	return true;

}

struct string *
generate_link_list_string_for_pager(const struct link_list *links)
{
	struct string *str = crtas("Links:\n", 7);
	if (str == NULL) {
		return false;
	}
	// Square brackets, colon and space (4) + longest size_t (20) + terminator (1)
#define LINK_PREFIX_SIZE 25
	char prefix[LINK_PREFIX_SIZE];
	size_t prefix_len;
	bool is_first_link = true;
	bool appended_type, appended_size, appended_duration;
	struct string *readable_size;
	for (size_t i = 0; i < links->len; ++i) {
		if ((links->list[i].url == NULL) || (links->list[i].url->len == 0)) {
			continue;
		}
		if (is_first_link == false) {
			if (catcs(str, '\n') == false) { goto error; }
		}
		is_first_link = false;
		// non-breaking space below the parentheses --------> ( )
		prefix_len = snprintf(prefix, LINK_PREFIX_SIZE, "[%zu]: ", i + 1);
#undef LINK_PREFIX_SIZE
		if (catas(str, prefix, prefix_len) == false) { goto error; }
		if (catss(str, links->list[i].url) == false) { goto error; }

		appended_type = false;
		if ((links->list[i].type != NULL) && (links->list[i].type->len != 0)) {
			if (catas(str, " (type: ", 8) == false) { goto error; }
			if (catss(str, links->list[i].type) == false) { goto error; }
			appended_type = true;
		}

		appended_size = false;
		if ((links->list[i].size != NULL) &&
		    (links->list[i].size->len != 0) &&
		    (strcmp(links->list[i].size->ptr, "0") != 0))
		{
			if (appended_type == true) {
				if (catas(str, ", size: ", 8) == false) { goto error; }
			} else {
				if (catas(str, " (size: ", 8) == false) { goto error; }
			}
			readable_size = convert_bytes_to_human_readable_size_string(links->list[i].size->ptr);
			if (readable_size == NULL) { goto error; }
			if (catss(str, readable_size) == false) { free_string(readable_size); goto error; }
			free_string(readable_size);
			appended_type = true;
		}

		appended_duration = false;
		if ((links->list[i].duration != NULL) &&
		    (links->list[i].duration->len != 0) &&
		    (strcmp(links->list[i].duration->ptr, "0") != 0))
		{
			if ((appended_type == true) || (appended_size == true)) {
				if (catas(str, ", duration: ", 12) == false) { goto error; }
			} else {
				if (catas(str, " (duration: ", 12) == false) { goto error; }
			}
			// TODO make duration readable too
			if (catss(str, links->list[i].duration) == false) { goto error; }
			appended_duration = true;
		}

		if ((appended_type == true) ||
		    (appended_size == true) ||
		    (appended_duration == true))
		{
			if (catcs(str, ')') == false) {
				goto error;
			}
		}

	}
	return str;
error:
	free_string(str);
	return NULL;
}

// Creates new URL without paths. For example:
//     http://example.org/feeds/atom.xml
// becomes:
//     http://example.org
static inline struct string *
strip_paths_from_url(const char *url, size_t url_len)
{
	const char *slash_pos = strchr(url, '/');
	if ((slash_pos == NULL) || (slash_pos == url)) {
		return crtas(url, url_len);
	}
	if ((*(slash_pos - 1) == ':') && (*(slash_pos + 1) == '/')) {
		slash_pos = strchr(slash_pos + 2, '/');
		if (slash_pos == NULL) {
			return crtas(url, url_len);
		}
	}
	struct string *result = crtas(url, slash_pos - url);
	if (result != NULL) {
		INFO("URL \"%s\" without paths looks like \"%s\".", url, result->ptr);
	} else {
		FAIL("Not enough memory for URL without paths!");
	}
	return result;
}

// Some URLs of item links are presented in relative form.
// For example:
//     (1)  /upload/podcast83.mp3
// or:
//     (2)  ../../images/image-194.jpg
// or:
//     (3)  #fnref:5
// Better to beautify these URLs by prepending feed url root
// in case (1) or by prepending item url with trailing slash
// in case (2) or by prepending item url without trailing slash
// in case (3). So it transforms to:
//     (1)  http://example.org/upload/podcast83.mp3
// or:
//     (2)  http://example.org/posts/83-new-podcast/../../images/image-194.jpg
// or:
//     (3)  http://example.org/posts/83-new-podcast#fnref:5
bool
complete_urls_of_links(struct link_list *links, sqlite3_stmt *res)
{
	INFO("Completing URLs of link list.");
	const char *feed_url = (const char *)sqlite3_column_text(res, ITEM_COLUMN_FEED_URL);
	if (feed_url == NULL) {
		FAIL("Feed URL of the item is unset!");
		return false;
	}
	size_t feed_url_len = strlen(feed_url);
	if (feed_url_len == 0) {
		FAIL("Feed URL of the item is empty!");
		return false;
	}
	struct string *url_without_paths = strip_paths_from_url(feed_url, feed_url_len);
	if (url_without_paths == NULL) {
		// Error message written by strip_paths_from_url.
		return false;
	}
	const char *item_link;
	size_t item_link_len;
	struct string *temp;
	const char *protocol_sep_pos;
	size_t protocol_name_len;
	for (size_t i = 0; i < links->len; ++i) {
		if (links->list[i].url->ptr[0] == '/') {
			temp = crtss(url_without_paths);
			if (temp == NULL) {
				free_string(url_without_paths);
				return false;
			}
			catss(temp, links->list[i].url);
			free_string(links->list[i].url);
			links->list[i].url = temp;
		} else {
			protocol_sep_pos = strstr(links->list[i].url->ptr, "://");
			if (protocol_sep_pos != NULL) {
				protocol_name_len = protocol_sep_pos - links->list[i].url->ptr - 1;
			} else if (strncmp(links->list[i].url->ptr, "mailto:", 7) == 0) {
				continue;
			} else if (strncmp(links->list[i].url->ptr, "tel:", 4) == 0) {
				continue;
			}
#define MAX_PROTOCOL_NAME_LEN 10
			if ((protocol_sep_pos == NULL) || (protocol_name_len > MAX_PROTOCOL_NAME_LEN)) {
#undef MAX_PROTOCOL_NAME_LEN
				item_link = (const char *)sqlite3_column_text(res, ITEM_COLUMN_LINK);
				if (item_link == NULL) { continue; }
				item_link_len = strlen(item_link);
				if (item_link_len == 0) { continue; }
				temp = crtas(item_link, item_link_len);
				if (temp == NULL) {
					free_string(url_without_paths);
					return false;
				}
				if (links->list[i].url->ptr[0] == '#') {
					if (temp->ptr[temp->len - 1] == '/') {
						temp->ptr[temp->len - 1] = '\0';
						--(temp->len);
					}
				} else {
					if (temp->ptr[temp->len - 1] != '/') {
						catcs(temp, '/');
					}
				}
				catss(temp, links->list[i].url);
				free_string(links->list[i].url);
				links->list[i].url = temp;
			}
		}
	}
	free_string(url_without_paths);
	return true;
}
