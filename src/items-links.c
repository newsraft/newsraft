#include "feedeater.h"

bool
add_another_url_to_trim_link_list(struct link_list *links, char *url, size_t url_len)
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

enum enclosure_field_number {
	ENCLOSURE_URL,
	ENCLOSURE_TYPE,
	ENCLOSURE_SIZE,
	ENCLOSURE_DURATION,
};

static inline bool
append_enclosures(struct link_list *links, sqlite3_stmt *res)
{
	char *text = (char *)sqlite3_column_text(res, ITEM_COLUMN_ENCLOSURES);
	if (text == NULL) {
		return true; // It is not an error because this item simply does not have link set.
	}
	char word[3333];
	size_t word_len = 0;
	int8_t field_num = ENCLOSURE_URL;
	char *iter = text;
	struct string **target;
	while (*iter != '\0') {
		if ((*iter == ' ') || (*iter == '\n')) {
			if (word_len != 0) {
				target = NULL;
				if (field_num == ENCLOSURE_URL) {
					if (add_another_url_to_trim_link_list(links, word, word_len) == false) {
						return false;
					}
				} else if (field_num == ENCLOSURE_TYPE) {
					target = &links->list[links->len - 1].type;
				} else if (field_num == ENCLOSURE_SIZE) {
					target = &links->list[links->len - 1].size;
				} else if (field_num == ENCLOSURE_DURATION) {
					target = &links->list[links->len - 1].duration;
				}
				if ((target != NULL) && (*target == NULL)) {
					*target = crtas(word, word_len);
					if (*target == NULL) {
						return false;
					}
				}
			}
			if (*iter == '\n') {
				field_num = ENCLOSURE_TYPE;
			} else {
				++field_num;
			}
			word_len = 0;
		} else {
			word[word_len++] = *iter;
		}
		++iter;
	}
	return true;
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
	if (append_enclosures(links, res) == false) {
		return false;
	}
	// TODO: APPEND LINKS OF HTML SUMMARY/CONTENT SOMEHOW
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
		prefix_len = snprintf(prefix, LINK_PREFIX_SIZE, "[%zu]: ", i + 1);
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
		if ((links->list[i].size != NULL) && (links->list[i].size->len != 0)) {
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
		if ((links->list[i].duration != NULL) && (links->list[i].duration->len != 0)) {
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
