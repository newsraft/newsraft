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

// See tests/complete_url.c to understand what it takes in and spews out.
char *
complete_url(const char *base, const char *rel)
{
	if (base == NULL || rel == NULL) {
		return NULL;
	}
	// Don't complete links with protocol scheme
	// Don't complete telephone links
	// Don't complete email links
	if (strstr(rel, "://") != NULL || strstr(rel, "tel:") == rel || strstr(rel, "mailto:") == rel) {
		return strdup(rel);
	}
	CURLU *link = curl_url();
	if (link == NULL) {
		return NULL;
	}
	if (curl_url_set(link, CURLUPART_URL, base, 0) != CURLUE_OK) {
		curl_url_cleanup(link);
		return NULL;
	}
	if (curl_url_set(link, CURLUPART_URL, rel, 0) != CURLUE_OK) {
		curl_url_cleanup(link);
		return NULL;
	}
	char *new_url = NULL;
	if (curl_url_get(link, CURLUPART_URL, &new_url, 0) != CURLUE_OK || new_url == NULL) {
		curl_free(new_url);
		curl_url_cleanup(link);
		return NULL;
	}
	char *complete = strdup(new_url);
	curl_free(new_url);
	curl_url_cleanup(link);
	return complete;
}

// Returns link index in the links list or -1 in case of failure.
int64_t
add_url_to_links_list(struct links_list *links, const char *url, size_t url_len)
{
	char *full_url = NULL;
	if (links->len > 0) {
		full_url = complete_url(links->ptr[0].url->ptr, url);
		if (full_url != NULL) {
			url = full_url;
			url_len = strlen(full_url);
		}
	}

	for (int64_t i = 0; i < (int64_t)links->len; ++i) {
		if (url_len == links->ptr[i].url->len && strncmp(url, links->ptr[i].url->ptr, url_len) == 0) {
			free(full_url);
			return i; // Don't add duplicate.
		}
	}
	struct link *tmp = realloc(links->ptr, sizeof(struct link) * (links->len + 1));
	if (tmp == NULL) {
		free(full_url);
		return -1;
	}

	int64_t index = (links->len)++;
	links->ptr = tmp;
	memset(&links->ptr[index], 0, sizeof(struct link));
	cpyas(&links->ptr[index].url, url, url_len);

	free(full_url);

	return links->ptr[index].url == NULL ? -1 : index;
}

static void
free_contents_of_link(const struct link *link)
{
	free_string(link->url);
	free_string(link->type);
	free_string(link->size);
	free_string(link->duration);
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

bool
add_item_attachments_to_links_list(struct links_list *links, sqlite3_stmt *res)
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

struct wstring *
generate_link_list_wstring_for_pager(struct config_context **ctx, const struct links_list *links)
{
	struct wstring *list = wcrtes(200);
	struct wstring *fmt_out = wcrtes(200);
	struct string *str = crtes(200);
	const struct wstring *link_fmt = get_cfg_wstring(ctx, CFG_ITEM_CONTENT_LINK_FORMAT);
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
