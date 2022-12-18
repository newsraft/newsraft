#include <string.h>
#include "newsraft.h"

#define MAX_METADATA_ENTRY_NAME_LENGTH 12
// Currently it is "contributors".

typedef int8_t entry_extra_id;
enum {
	NO_EXTRA_DATA = 0,
	REQUIRES_INLINING = 1,
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
	bool (*append_handler)(struct render_blocks_list *blocks, sqlite3_stmt *res, const struct data_entry *entry);
};

static bool
append_plain_text(struct render_blocks_list *blocks, sqlite3_stmt *res, const struct data_entry *entry)
{
	const char *text = (char *)sqlite3_column_text(res, entry->column);
	if (text == NULL) {
		return true; // It is not an error because this item simply does not have value set.
	}
	const size_t text_len = strlen(text);
	if (text_len == 0) {
		return true; // It is not an error because this item simply does not have value set.
	}
	if (join_render_block(blocks, entry->tooltip, entry->tooltip_len, TEXT_PLAIN) == false) {
		return false;
	}
	if ((entry->feature_mask & REQUIRES_INLINING) == 0) {
		if (join_render_block(blocks, text, text_len, TEXT_PLAIN) == false) {
			return false;
		}
	} else {
		struct string *title = crtas(text, text_len);
		if (title == NULL) {
			return false;
		}
		inlinefy_string(title);
		if (join_render_block(blocks, title->ptr, title->len, TEXT_PLAIN) == false) {
			free_string(title);
			return false;
		}
		free_string(title);
	}
	return join_render_separator(blocks);
}

static bool
append_max_content(struct render_blocks_list *blocks, sqlite3_stmt *res, const struct data_entry *entry)
{
	(void)entry;
	const char *content = (char *)sqlite3_column_text(res, ITEM_COLUMN_CONTENT);
	struct string *text = crtes();
	if (text == NULL) {
		goto undo0;
	}
	struct string *type = crtas("text/plain", 10);
	if (type == NULL) {
		goto undo1;
	}
	if (get_largest_piece_from_item_content(content, text, type) == false) {
		goto undo2;
	}
	if (text->len == 0) {
		// There were no texts in the content, let's try to search in
		// the descriptions for item's attachments.
		const char *attachments = (char *)sqlite3_column_text(res, ITEM_COLUMN_ATTACHMENTS);
		if (get_largest_piece_from_item_attachments(attachments, text, type) == false) {
			goto undo2;
		}
	}
	if (text->len != 0) {
		if (join_render_separator(blocks) == false) {
			goto undo2;
		}
		if (join_render_block(blocks, text->ptr, text->len, get_content_type_by_string(type->ptr)) == false) {
			goto undo2;
		}
	}
	free_string(text);
	free_string(type);
	return true;
undo2:
	free_string(type);
undo1:
	free_string(text);
undo0:
	return false;
}


static bool
append_date(struct render_blocks_list *blocks, sqlite3_stmt *res, const struct data_entry *entry)
{
	int64_t date = sqlite3_column_int64(res, entry->column);
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
	if (join_render_block(blocks, date_entry->ptr, date_entry->len, TEXT_PLAIN) == false) {
		free_string(date_entry);
		return false;
	}
	free_string(date_entry);
	return join_render_separator(blocks);
}

static bool
append_persons(struct render_blocks_list *blocks, sqlite3_stmt *res, const struct data_entry *entry)
{
	const char *serialized_persons = (char *)sqlite3_column_text(res, ITEM_COLUMN_PERSONS);
	if (serialized_persons == NULL) {
		return true; // Ignore empty persons >:-D
	}
	struct string *persons;
	if (entry->feature_mask == PERSON_AUTHOR) {
		persons = deserialize_persons_string(serialized_persons, "author");
	} else if (entry->feature_mask == PERSON_CONTRIBUTOR) {
		persons = deserialize_persons_string(serialized_persons, "contributor");
	} else if (entry->feature_mask == PERSON_EDITOR) {
		persons = deserialize_persons_string(serialized_persons, "editor");
	} else {
		persons = NULL;
	}
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
	if (join_render_block(blocks, block_text->ptr, block_text->len, TEXT_PLAIN) == false) {
		free_string(block_text);
		return false;
	}
	free_string(block_text);
	return join_render_separator(blocks);
}

// ATTENTION! Maximal length of meta data entry name has to be reflected in MAX_METADATA_ENTRY_NAME_LENGTH.
static const struct data_entry entries[] = {
	{"feed-url",     "Feed: ",          6, ITEM_COLUMN_FEED_URL,         NO_EXTRA_DATA,      &append_plain_text},
	{"title",        "Title: ",         7, ITEM_COLUMN_TITLE,            REQUIRES_INLINING,  &append_plain_text},
	{"link",         "Link: ",          6, ITEM_COLUMN_LINK,             NO_EXTRA_DATA,      &append_plain_text},
	{"published",    "Published: ",    11, ITEM_COLUMN_PUBLICATION_DATE, 0,                  &append_date},
	{"updated",      "Updated: ",       9, ITEM_COLUMN_UPDATE_DATE,      0,                  &append_date},
	{"authors",      "Authors: ",       9, 0,                            PERSON_AUTHOR,      &append_persons},
	{"contributors", "Contributors: ", 14, 0,                            PERSON_CONTRIBUTOR, &append_persons},
	{"editors",      "Editors: ",       9, 0,                            PERSON_EDITOR,      &append_persons},
	{"max-content",  NULL,              0, 0,                            0,                  &append_max_content},
	{NULL,           NULL,              0, 0,                            0,                  NULL},
};

bool
join_render_blocks_of_item_data(struct render_blocks_list *blocks, sqlite3_stmt *res)
{
	char entry[MAX_METADATA_ENTRY_NAME_LENGTH + 1];
	size_t entry_len = 0;
	const struct string *content_order = get_cfg_string(CFG_ITEM_FORMATION_ORDER);
	const char *i = content_order->ptr;
	while (true) {
		if ((*i == ',') || (*i == '\0')) {
			for (size_t j = 0; entries[j].field != NULL; ++j) {
				if (strncmp(entry, entries[j].field, entry_len) == 0) {
					if (entries[j].append_handler(blocks, res, entries + j) == false) {
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
