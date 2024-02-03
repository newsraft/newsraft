#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "newsraft.h"

static inline int
convert_bytes_to_human_readable_size_string(char *dest, size_t dest_size, const char *value)
{
	float size = -1;
	if (sscanf(value, "%f", &size) != 1 || size < 0) {
		FAIL("Failed to convert \"%s\" to size number!", value);
		return 0;
	}
	if (size < 1100) {
		return snprintf(dest, dest_size, "%.0f bytes", size);
	} else if (size < 1100000) {
		return snprintf(dest, dest_size, "%.1f KB", size / 1000);
	} else if (size < 1100000000) {
		return snprintf(dest, dest_size, "%.1f MB", size / 1000000);
	}
	return snprintf(dest, dest_size, "%.2f GB", size / 1000000000);
}

static inline int
convert_seconds_to_human_readable_duration_string(char *dest, size_t dest_size, const char *value)
{
	float duration = -1;
	if (sscanf(value, "%f", &duration) != 1 || duration < 0) {
		FAIL("Failed to convert \"%s\" to duration number!", value);
		return 0;
	}
	if (duration < 90) {
		return snprintf(dest, dest_size, "%.0f seconds", duration);
	} else if (duration < 4000) {
		return snprintf(dest, dest_size, "%.1f minutes", duration / 60);
	}
	return snprintf(dest, dest_size, "%.1f hours", duration / 3600);
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
free_links_list(const struct links_list *links)
{
	if (links->ptr != NULL) {
		for (size_t i = 0; i < links->len; ++i) {
			free_contents_of_link(&(links->ptr[i]));
		}
		free(links->ptr);
	}
}

static inline bool
append_raw_link(struct links_list *links, sqlite3_stmt *res, items_column_id column)
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
			if (cpyas(&another_link.url, entry->ptr + 4, entry->len - 4) == false) {
				goto error;
			}
		} else if (strncmp(entry->ptr, "type=", 5) == 0) {
			if (cpyas(&another_link.type, entry->ptr + 5, entry->len - 5) == false) {
				goto error;
			}
		} else if (strncmp(entry->ptr, "size=", 5) == 0) {
			if (cpyas(&another_link.size, entry->ptr + 5, entry->len - 5) == false) {
				goto error;
			}
		} else if (strncmp(entry->ptr, "duration=", 9) == 0) {
			if (cpyas(&another_link.duration, entry->ptr + 9, entry->len - 9) == false) {
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

struct wstring *
generate_link_list_wstring_for_pager(const struct links_list *links)
{
	struct wstring *list = wcrtes(200);
	struct wstring *fmt_out = wcrtes(200);
	struct string *str = crtes(200);
	const struct wstring *link_fmt = get_cfg_wstring(CFG_ITEM_CONTENT_LINK_FORMAT);
#define CONVERT_OUT_SIZE 200
	char convert_out[CONVERT_OUT_SIZE];
	int convert_len;
	for (size_t i = 0; i < links->len; ++i) {
		if (links->ptr[i].url == NULL || links->ptr[i].url->len == 0) {
			continue;
		}
		if (cpyss(&str, links->ptr[i].url) == false) {
			goto error;
		}
		bool parentheses_are_open = false;

		if ((links->ptr[i].type != NULL) && (links->ptr[i].type->len != 0)) {
			if (catas(str, " (type: ", 8) == false) { goto error; }
			if (catss(str, links->ptr[i].type) == false) { goto error; }
			parentheses_are_open = true;
		}

		if ((links->ptr[i].size != NULL)
			&& (links->ptr[i].size->len != 0)
			&& (strcmp(links->ptr[i].size->ptr, "0") != 0))
		{
			convert_len = convert_bytes_to_human_readable_size_string(convert_out, CONVERT_OUT_SIZE, links->ptr[i].size->ptr);
			if (convert_len > 0 && convert_len < CONVERT_OUT_SIZE) {
				if (catas(str, parentheses_are_open ? ", size: " : " (size: ", 8) == false) { goto error; }
				if (catas(str, convert_out, convert_len) == false) { goto error; }
				parentheses_are_open = true;
			}
		}

		if ((links->ptr[i].duration != NULL)
			&& (links->ptr[i].duration->len != 0)
			&& (strcmp(links->ptr[i].duration->ptr, "0") != 0))
		{
			convert_len = convert_seconds_to_human_readable_duration_string(convert_out, CONVERT_OUT_SIZE, links->ptr[i].duration->ptr);
			if (convert_len > 0 && convert_len < CONVERT_OUT_SIZE) {
				if (catas(str, parentheses_are_open ? ", duration: " : " (duration: ", 12) == false) { goto error; }
				if (catas(str, convert_out, convert_len) == false) { goto error; }
				parentheses_are_open = true;
			}
		}

		if (parentheses_are_open == true && catcs(str, ')') == false) goto error;

		struct format_arg link_fmt_args[] = {
			{L'i',  L'd',  {.i = i + 1   }},
			{L'l',  L's',  {.s = str->ptr}},
			{L'\0', L'\0', {.i = 0       }}, // terminator
		};
		do_format(fmt_out, link_fmt->ptr, link_fmt_args);
		wcatss(list, fmt_out);
	}
	free_wstring(fmt_out);
	free_string(str);
	return list;
error:
	free_wstring(list);
	free_wstring(fmt_out);
	free_string(str);
	return NULL;
}

// Convert relative URLs of links to absolute form. For example:
// (1) /upload/podcast83.mp3      => http://example.org/upload/podcast83.mp3
// (2) ../../images/image-194.jpg => http://example.org/post13/../../images/image-194.jpg
// (3) image-195.jpg              => http://example.org/post13/image-195.jpg
bool
complete_urls_of_links(struct links_list *links)
{
	INFO("Completing URLs of links list.");
	CURLU *h = curl_url();
	if (h == NULL) {
		FAIL("Not enough memory for completing URLs of links list!");
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
		if (strstr(links->ptr[i].url->ptr, "://") != NULL) {
			continue; // This URL has protocol scheme, it's most likely absolute.
		}
		if (curl_url_set(h, CURLUPART_URL, links->ptr[0].url->ptr, 0) != CURLUE_OK) {
			continue; // This URL is broken, leave it alone.
		}
		if (curl_url_set(h, CURLUPART_URL, links->ptr[i].url->ptr, 0) != CURLUE_OK) {
			continue; // This URL is broken, leave it alone.
		}
		if (curl_url_get(h, CURLUPART_URL, &url, 0) != CURLUE_OK) {
			continue; // This URL is broken, leave it alone.
		}
		INFO("Completed \"%s\" to \"%s\".", links->ptr[i].url->ptr, url);
		if (cpyas(&links->ptr[i].url, url, strlen(url)) == false) {
			curl_free(url);
			curl_url_cleanup(h);
			return false;
		}
		curl_free(url);
	}
	curl_url_cleanup(h);
	return true;
}
