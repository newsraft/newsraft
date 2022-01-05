#include "feedeater.h"

bool
expand_trim_link_list_by_one_element(struct trim_link_list *links)
{
	struct trim_link *temp = realloc(links->list, sizeof(struct trim_link) * (links->len + 1));
	if (temp == NULL) {
		return false;
	}
	links->list = temp;
	links->list[links->len].url = NULL;
	links->list[links->len].type = NULL;
	links->list[links->len].size = NULL;
	links->list[links->len].duration = NULL;
	++(links->len);
	return true;
}

void
free_trim_link_list(const struct trim_link_list *links)
{
	for (size_t i = 0; i < links->len; ++i) {
		free_string(links->list[i].url);
		free_string(links->list[i].type);
		free_string(links->list[i].size);
		free_string(links->list[i].duration);
	}
	free(links->list);
}

static inline bool
append_raw_link(struct trim_link_list *links, sqlite3_stmt *res, enum item_column column)
{
	char *text = (char *)sqlite3_column_text(res, column);
	if (text == NULL) {
		return true; // It is not an error because this item simply does not have link set.
	}
	size_t text_len = strlen(text);
	if (text_len == 0) {
		return true; // It is not an error because this item simply does not have link set.
	}
	if (expand_trim_link_list_by_one_element(links) == false) {
		return false;
	}
	links->list[links->len - 1].url = create_string(text, text_len);
	if (links->list[links->len - 1].url == NULL) {
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
append_enclosures(struct trim_link_list *links, sqlite3_stmt *res)
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
			target = NULL;
			if (field_num == ENCLOSURE_URL) {
				if (expand_trim_link_list_by_one_element(links) == false) {
					return false;
				}
				target = &links->list[links->len - 1].url;
			} else if (field_num == ENCLOSURE_TYPE) {
				target = &links->list[links->len - 1].type;
			} else if (field_num == ENCLOSURE_SIZE) {
				target = &links->list[links->len - 1].size;
			} else if (field_num == ENCLOSURE_DURATION) {
				target = &links->list[links->len - 1].duration;
			}
			if ((target != NULL) && (*target == NULL)) {
				*target = create_string(word, word_len);
				if (*target == NULL) {
					return false;
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
populate_link_list_with_links_of_item(struct trim_link_list *links, sqlite3_stmt *res)
{
	if (append_raw_link(links, res, ITEM_COLUMN_LINK) == false) {
		return false;
	}
	if (append_raw_link(links, res, ITEM_COLUMN_COMMENTS) == false) {
		return false;
	}
	if (append_enclosures(links, res) == false) {
		return false;
	}
	// TODO: APPEND LINKS OF HTML SUMMARY/CONTENT SOMEHOW
	return true;

}
