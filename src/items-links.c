#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "newsraft.h"

static inline struct string *
convert_bytes_to_human_readable_size_string(const char *value)
{
	float size;
	if (sscanf(value, "%f", &size) != 1) {
		FAIL("Can't convert \"%s\" string to float!", value);
		return NULL;
	}
	if (size < 0) {
		FAIL("Number of bytes turned out to be negative!");
		return NULL;
	}
	// longest float integral part (40) +
	// point (1) +
	// two digits after point (2) +
	// space (1) +
	// longest name of data measure (5) +
	// null terminator (1) +
	// for luck (50) = 100
	char human_readable[100];
	int length;
	if (size < 1100) {
		length = sprintf(human_readable, "%.0f bytes", size);
	} else if (size < 1100000) {
		length = sprintf(human_readable, "%.1f KB", size / 1000);
	} else if (size < 1100000000) {
		length = sprintf(human_readable, "%.1f MB", size / 1000000);
	} else {
		length = sprintf(human_readable, "%.2f GB", size / 1000000000);
	}
	if (length < 0) {
		FAIL("Failed to write size string to buffer!");
		return NULL;
	}
	return crtas(human_readable, length);
}

static inline struct string *
convert_seconds_to_human_readable_duration_string(const char *value)
{
	float duration;
	if (sscanf(value, "%f", &duration) != 1) {
		FAIL("Can't convert \"%s\" string to float!", value);
		return NULL;
	}
	if (duration < 0) {
		FAIL("Number of seconds turned out to be negative!");
		return NULL;
	}
	// longest float integral part (40) +
	// point (1) +
	// one digit after point (1) +
	// space (1) +
	// longest name of data measure (7) +
	// null terminator (1) +
	// for luck (50) = 101
	char human_readable[101];
	int length;
	if (duration < 90) {
		length = sprintf(human_readable, "%.0f seconds", duration);
	} else if (duration < 4000) {
		length = sprintf(human_readable, "%.1f minutes", duration / 60);
	} else {
		length = sprintf(human_readable, "%.1f hours", duration / 3600);
	}
	if (length < 0) {
		FAIL("Failed to write duration string to buffer!");
		return NULL;
	}
	return crtas(human_readable, length);
}

// Return codes correspond to:
// [0; INT64_MAX] link was added successfully with that index
// {-1}           memory failure
int64_t
add_another_url_to_trim_links_list(struct links_list *links, const char *url, size_t url_len)
{
	for (int64_t i = 0; i < (int64_t)links->len; ++i) {
		if ((url_len == links->ptr[i].url->len) && (strncmp(url, links->ptr[i].url->ptr, url_len) == 0)) {
			// Don't add duplicate.
			return i;
		}
	}
	struct link *temp = realloc(links->ptr, sizeof(struct link) * (links->len + 1));
	if (temp == NULL) {
		return -1;
	}
	size_t index = (links->len)++;
	links->ptr = temp;
	links->ptr[index].url = crtas(url, url_len);
	links->ptr[index].type = NULL;
	links->ptr[index].size = NULL;
	links->ptr[index].duration = NULL;
	if (links->ptr[index].url == NULL) {
		return -1;
	}
	return index;
}

static void
free_contents_of_link(const struct link *link)
{
	free_string(link->url);
	free_string(link->type);
	free_string(link->size);
	free_string(link->duration);
}

void
free_trim_link_list(const struct links_list *links)
{
	if (links->ptr != NULL) {
		for (size_t i = 0; i < links->len; ++i) {
			free_contents_of_link(&(links->ptr[i]));
		}
		free(links->ptr);
	}
}

static inline bool
append_raw_link(struct links_list *links, sqlite3_stmt *res, enum item_column column)
{
	const char *text = (char *)sqlite3_column_text(res, column);
	if (text != NULL) {
		size_t text_len = strlen(text);
		if (text_len != 0) {
			if (add_another_url_to_trim_links_list(links, text, text_len) < 0) {
				return false;
			}
		}
	}
	return true; // It's not an error because this item simply doesn't have link set.
}

static inline bool
add_another_link_to_trim_link_list(struct links_list *links, const struct link *link)
{
	if ((link->url == NULL) || (link->url->len == 0)) {
		free_contents_of_link(link);
		return true; // Ignore empty links.
	}
	for (int64_t i = 0; i < (int64_t)links->len; ++i) {
		if (strcmp(link->url->ptr, links->ptr[i].url->ptr) == 0) {
			// Don't add duplicate.
			free_contents_of_link(link);
			return true;
		}
	}
	struct link *temp = realloc(links->ptr, sizeof(struct link) * (links->len + 1));
	if (temp == NULL) {
		free_contents_of_link(link);
		return false;
	}
	links->ptr = temp;
	size_t index = (links->len)++;
	links->ptr[index].url = link->url;
	links->ptr[index].type = link->type;
	links->ptr[index].size = link->size;
	links->ptr[index].duration = link->duration;
	return true;
}

static inline bool
append_attachments(struct links_list *links, sqlite3_stmt *res)
{
	const char *text = (const char *)sqlite3_column_text(res, ITEM_COLUMN_ATTACHMENTS);
	if (text == NULL) {
		return true; // It is not an error because this item simply does not have attachments set.
	}
	struct deserialize_stream *s = open_deserialize_stream(text);
	if (s == NULL) {
		return false;
	}
	struct link another_link = {0};
	const struct string *entry = get_next_entry_from_deserialize_stream(s);
	while (entry != NULL) {
		if (strcmp(entry->ptr, "^") == 0) {
			add_another_link_to_trim_link_list(links, &another_link);
			memset(&another_link, 0, sizeof(struct link));
		} else if (strncmp(entry->ptr, "url=", 4) == 0) {
			if (crtas_or_cpyas(&another_link.url, entry->ptr + 4, entry->len - 4) == false) {
				goto error;
			}
		} else if (strncmp(entry->ptr, "type=", 5) == 0) {
			if (crtas_or_cpyas(&another_link.type, entry->ptr + 5, entry->len - 5) == false) {
				goto error;
			}
		} else if (strncmp(entry->ptr, "size=", 5) == 0) {
			if (crtas_or_cpyas(&another_link.size, entry->ptr + 5, entry->len - 5) == false) {
				goto error;
			}
		} else if (strncmp(entry->ptr, "duration=", 9) == 0) {
			if (crtas_or_cpyas(&another_link.duration, entry->ptr + 9, entry->len - 9) == false) {
				goto error;
			}
		}
		entry = get_next_entry_from_deserialize_stream(s);
	}
	add_another_link_to_trim_link_list(links, &another_link);
	close_deserialize_stream(s);
	return true;
error:
	free_contents_of_link(&another_link);
	close_deserialize_stream(s);
	return false;
}

bool
populate_link_list_with_links_of_item(struct links_list *links, sqlite3_stmt *res)
{
	if (append_raw_link(links, res, ITEM_COLUMN_LINK) == true) {
		if (append_attachments(links, res) == true) {
			return true;
		}
	}
	return false;
}

struct string *
generate_link_list_string_for_pager(const struct links_list *links)
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
	struct string *readable_string = NULL;
	for (size_t i = 0; i < links->len; ++i) {
		if ((links->ptr[i].url == NULL) || (links->ptr[i].url->len == 0)) {
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
		if (catss(str, links->ptr[i].url) == false) { goto error; }

		appended_type = false;
		if ((links->ptr[i].type != NULL) && (links->ptr[i].type->len != 0)) {
			if (catas(str, " (type: ", 8) == false) { goto error; }
			if (catss(str, links->ptr[i].type) == false) { goto error; }
			appended_type = true;
		}

		appended_size = false;
		if ((links->ptr[i].size != NULL)
			&& (links->ptr[i].size->len != 0)
			&& (strcmp(links->ptr[i].size->ptr, "0") != 0))
		{
			readable_string = convert_bytes_to_human_readable_size_string(links->ptr[i].size->ptr);
			if (readable_string != NULL) {
				if (appended_type == true) {
					if (catas(str, ", size: ", 8) == false) { goto error; }
				} else {
					if (catas(str, " (size: ", 8) == false) { goto error; }
				}
				if (catss(str, readable_string) == false) { goto error; }
				free_string(readable_string);
				readable_string = NULL;
				appended_size = true;
			}
		}

		appended_duration = false;
		if ((links->ptr[i].duration != NULL)
			&& (links->ptr[i].duration->len != 0)
			&& (strcmp(links->ptr[i].duration->ptr, "0") != 0))
		{
			readable_string = convert_seconds_to_human_readable_duration_string(links->ptr[i].duration->ptr);
			if (readable_string != NULL) {
				if ((appended_type == true) || (appended_size == true)) {
					if (catas(str, ", duration: ", 12) == false) { goto error; }
				} else {
					if (catas(str, " (duration: ", 12) == false) { goto error; }
				}
				if (catss(str, readable_string) == false) { goto error; }
				free_string(readable_string);
				readable_string = NULL;
				appended_duration = true;
			}
		}

		if ((appended_type == true)
			|| (appended_size == true)
			|| (appended_duration == true))
		{
			if (catcs(str, ')') == false) {
				goto error;
			}
		}

	}
	return str;
error:
	free_string(readable_string);
	free_string(str);
	return NULL;
}

// Some URLs of item links are presented in relative form. For example:
//     (1)  /upload/podcast83.mp3
// or:
//     (2)  ../../images/image-194.jpg
// In this function we beautify these URLs by prepending feed url hostname in
// case (1) and by prepending full feed url with trailing slash in case (2).
bool
complete_urls_of_links(struct links_list *links, sqlite3_stmt *res)
{
	INFO("Completing URLs of link list.");
	const char *feed_url = (char *)sqlite3_column_text(res, ITEM_COLUMN_FEED_URL);
	if ((feed_url == NULL) || (strlen(feed_url) == 0)) {
		FAIL("Feed URL of the item is empty!");
		return false;
	}
	CURLU *h = curl_url();
	if (h == NULL) {
		FAIL("Not enough memory for parsing URL!");
		return false;
	}
	char *url;
	for (size_t i = 0; i < links->len; ++i) {
		if (strncmp(links->ptr[i].url->ptr, "mailto:", 7) == 0) {
			continue; // This is an email URL, leave it as is.
		}
		if (strncmp(links->ptr[i].url->ptr, "tel:", 4) == 0) {
			continue; // This is a telephone URL, leave it as is.
		}
		if (curl_url_set(h, CURLUPART_URL, feed_url, 0) != CURLUE_OK) {
			continue; // This URL is broken, leave it alone.
		}
		if (curl_url_set(h, CURLUPART_URL, links->ptr[i].url->ptr, 0) != CURLUE_OK) {
			continue; // This URL is broken, leave it alone.
		}
		if (curl_url_get(h, CURLUPART_URL, &url, 0) != CURLUE_OK) {
			continue; // This URL is broken, leave it alone.
		}
		if (cpyas(links->ptr[i].url, url, strlen(url)) == false) {
			curl_free(url);
			curl_url_cleanup(h);
			return false;
		}
		curl_free(url);
	}
	curl_url_cleanup(h);
	return true;
}
