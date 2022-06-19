#include <string.h>
#include "newsraft.h"

#define MAX_METADATA_ENTRY_NAME_LENGTH 12
// Currently it is "contributors".

typedef int8_t entry_extra_id;
enum {
	NO_EXTRA_DATA = 0,
	TYPE_HEADER = 1,
	PREPEND_SEPARATOR = 2,
	PERSON_AUTHOR = 4,
	PERSON_CONTRIBUTOR = 8,
	PERSON_EDITOR = 16,
};

struct data_entry {
	const char *const field;      // Name of field to match in config_contents_meta_data.
	const char *const tooltip;    // String to write before entry data.
	const size_t tooltip_len;
	const items_column_id column; // Column index from which to take data.
	const entry_extra_id feature_mask;
	bool (*append_handler)(struct render_block **list, sqlite3_stmt *res, const struct data_entry *entry);
};

static bool
append_text(struct render_block **list, sqlite3_stmt *res, const struct data_entry *entry)
{
	const char *text = (const char *)sqlite3_column_text(res, entry->column);
	if (text == NULL) {
		return true; // It is not an error because this item simply does not have value set.
	}
	const size_t text_len = strlen(text);
	if (text_len == 0) {
		return true; // It is not an error because this item simply does not have value set.
	}
	if ((entry->feature_mask & PREPEND_SEPARATOR) != 0) {
		if (join_render_separator(list) == false) {
			return false;
		}
	}
	if ((entry->tooltip != NULL) && (entry->tooltip_len != 0)) {
		if (join_render_block(list, entry->tooltip, entry->tooltip_len, "text/plain", 10) == false) {
			return false;
		}
	}
	if ((entry->feature_mask & TYPE_HEADER) != 0) {
		const char *type_separator = strchr(text, ';');
		if (type_separator == NULL) {
			return false;
		}
		const size_t type_len = type_separator - text;
		if (type_len > MIME_TYPE_LENGTH_LIMIT) {
			return false;
		}
		const char *real_text = type_separator + 1;
		const size_t real_text_len = text_len - (type_len + 1);
		char type[MIME_TYPE_LENGTH_LIMIT + 1];
		memcpy(type, text, type_len);
		type[type_len] = '\0';
		if (join_render_block(list, real_text, real_text_len, type, type_len) == false) {
			return false;
		}
	} else {
		if (join_render_block(list, text, text_len, "text/plain", 10) == false) {
			return false;
		}
	}
	if (join_render_separator(list) == false) {
		return false;
	}
	return true;
}

static bool
append_max_content(struct render_block **list, sqlite3_stmt *res, const struct data_entry *entry)
{
	(void)entry;
	const char *summary = (char *)sqlite3_column_text(res, ITEM_COLUMN_SUMMARY);
	size_t summary_len;
	if (summary == NULL) {
		summary_len = 0;
	} else {
		summary_len = strlen(summary);
	}
	const char *content = (char *)sqlite3_column_text(res, ITEM_COLUMN_CONTENT);
	size_t content_len;
	if (content == NULL) {
		content_len = 0;
	} else {
		content_len = strlen(content);
	}
	if (summary_len > content_len) {
		content = summary;
		content_len = summary_len;
	}
	if ((content == NULL) || (content_len == 0)) {
		return true;
	}
	const char *type_separator = strchr(content, ';');
	if (type_separator == NULL) {
		return false;
	}
	const size_t type_len = type_separator - content;
	if (type_len > MIME_TYPE_LENGTH_LIMIT) {
		return false;
	}
	const char *real_content = content + (type_len + 1);
	const size_t real_content_len = content_len - (type_len + 1);
	char type[MIME_TYPE_LENGTH_LIMIT + 1];
	memcpy(type, content, type_len);
	type[type_len] = '\0';
	if (join_render_separator(list) == false) {
		return false;
	}
	if (join_render_block(list, real_content, real_content_len, type, type_len) == false) {
		return false;
	}
	return true;
}


static bool
append_date(struct render_block **list, sqlite3_stmt *res, const struct data_entry *entry)
{
	time_t date = (time_t)sqlite3_column_int64(res, entry->column);
	if (date == 0) {
		return true; // It is not an error because this item simply does not have this date set.
	}
	struct string *date_str = get_config_date_str(date, CFG_CONTENT_DATE_FORMAT);
	if (date_str == NULL) {
		return false;
	}
	struct string *date_entry = crtas(entry->tooltip, entry->tooltip_len);
	if (date_entry == NULL) {
		free_string(date_str);
		return false;
	}
	if (catss(date_entry, date_str) == false) {
		free_string(date_str);
		free_string(date_entry);
		return false;
	}
	free_string(date_str);
	if (join_render_block(list, date_entry->ptr, date_entry->len, "text/plain", 10) == false) {
		free_string(date_entry);
		return false;
	}
	free_string(date_entry);
	if (join_render_separator(list) == false) {
		return false;
	}
	return true;
}

static bool
append_persons(struct render_block **list, sqlite3_stmt *res, const struct data_entry *entry)
{
	const char *serialized_persons = (char *)sqlite3_column_text(res, ITEM_COLUMN_PERSONS);
	if (serialized_persons == NULL) {
		return true; // Ignore empty persons >:-D
	}
	const char *person_type;
	if (entry->feature_mask == PERSON_AUTHOR) {
		person_type = "author";
	} else if (entry->feature_mask == PERSON_CONTRIBUTOR) {
		person_type = "contributor";
	} else if (entry->feature_mask == PERSON_EDITOR) {
		person_type = "editor";
	} else {
		return true; // Ignore unknown type of persons >:-D
	}
	struct string *persons = deserialize_persons_string(serialized_persons, person_type);
	if ((persons == NULL) || (persons->len == 0)) {
		free_string(persons);
		return true; // There is nothing here that we were looking for.
	}
	struct string *block_text = crtas(entry->tooltip, entry->tooltip_len);
	if (block_text == NULL) {
		free_string(persons);
		return false;
	}
	if (catss(block_text, persons) == false) {
		free_string(persons);
		free_string(block_text);
		return false;
	}
	free_string(persons);
	if (join_render_block(list, block_text->ptr, block_text->len, "text/plain", 10) == false) {
		free_string(block_text);
		return false;
	}
	free_string(block_text);
	if (join_render_separator(list) == false) {
		return false;
	}
	return true;
}

// ATTENTION! Maximal length of meta data entry name has to be reflected in MAX_METADATA_ENTRY_NAME_LENGTH.
static const struct data_entry entries[] = {
	{"feed-url",     "Feed: ",          6, ITEM_COLUMN_FEED_URL,         NO_EXTRA_DATA,                 &append_text},
	{"title",        "Title: ",         7, ITEM_COLUMN_TITLE,            TYPE_HEADER,                   &append_text},
	/* {"categories",   "Categories: ",   12, ITEM_COLUMN_CATEGORIES,   false}, */
	{"link",         "Link: ",          6, ITEM_COLUMN_LINK,             NO_EXTRA_DATA,                 &append_text},
	{"published",    "Published: ",    11, ITEM_COLUMN_PUBLICATION_DATE, 0,                             &append_date},
	{"updated",      "Updated: ",       9, ITEM_COLUMN_UPDATE_DATE,      0,                             &append_date},
	{"authors",      "Authors: ",       9, 0,                            PERSON_AUTHOR,                 &append_persons},
	{"contributors", "Contributors: ", 14, 0,                            PERSON_CONTRIBUTOR,            &append_persons},
	{"editors",      "Editors: ",       9, 0,                            PERSON_EDITOR,                 &append_persons},
	{"summary",      NULL,              0, ITEM_COLUMN_SUMMARY,          TYPE_HEADER|PREPEND_SEPARATOR, &append_text},
	{"content",      NULL,              0, ITEM_COLUMN_CONTENT,          TYPE_HEADER|PREPEND_SEPARATOR, &append_text},
	{"max-content",  NULL,              0, 0,                            0,                             &append_max_content},
	{NULL,           NULL,              0, 0,                            0,                             NULL},
};

bool
join_render_blocks_of_item_data(struct render_block **list, sqlite3_stmt *res)
{
	char entry[MAX_METADATA_ENTRY_NAME_LENGTH + 1];
	size_t entry_len = 0;
	const struct string *content_order = get_cfg_string(CFG_ITEM_FORMATION_ORDER);
	const char *i = content_order->ptr;
	while (true) {
		if ((*i == ',') || (*i == '\0')) {
			for (size_t j = 0; entries[j].field != NULL; ++j) {
				if (strncmp(entry, entries[j].field, entry_len) == 0) {
					if (entries[j].append_handler(list, res, entries + j) == false) {
						return false;
					}
					break;
				}
			}
			if (*i == '\0') {
				break;
			}
			entry_len = 0;
		} else if (entry_len != MAX_METADATA_ENTRY_NAME_LENGTH) {
			entry[entry_len++] = *i;
		}
		++i;
	}
	return true;
}
