#include <string.h>
#include "newsraft.h"

#define MAX_METADATA_ENTRY_NAME_LENGTH 12
// Currently it is "contributors".

struct data_entry {
	const char *const field;      // Name of field to match in config_contents_meta_data.
	const char *const tooltip;    // String to write before entry data.
	const size_t tooltip_len;
	const items_column_id column; // Column index from which to take data.
	bool (*append_handler)(struct render_blocks_list *blocks, sqlite3_stmt *res, const struct data_entry *entry);
};

static bool
append_text_line(struct render_blocks_list *blocks, sqlite3_stmt *res, const struct data_entry *entry)
{
	const char *text = (char *)sqlite3_column_text(res, entry->column);
	if (text == NULL) {
		return true; // It is not an error because this item simply does not have value set.
	}
	const size_t text_len = strlen(text);
	if (text_len == 0) {
		return true; // It is not an error because this item simply does not have value set.
	}
	struct string *data = crtas(entry->tooltip, entry->tooltip_len);
	if (data == NULL) {
		return false;
	}
	if (catas(data, text, text_len) == false) {
		goto error;
	}
	inlinefy_string(data);
	if (join_render_block(blocks, data->ptr, data->len, TEXT_RAW) == false) {
		goto error;
	}
	free_string(data);
	return join_render_separator(blocks);
error:
	free_string(data);
	return false;
}

static bool
append_max_content(struct render_blocks_list *blocks, sqlite3_stmt *res, const struct data_entry *entry)
{
	(void)entry;
	const char *content = (char *)sqlite3_column_text(res, ITEM_COLUMN_CONTENT);
	struct string *text = crtes();
	if (text == NULL) {
		return false;
	}
	render_block_format type = TEXT_PLAIN;
	if (get_largest_piece_from_item_content(content, text, &type) == false) {
		goto error;
	}
	if (text->len == 0) {
		// There were no texts in the content, let's try to search in
		// the descriptions for item's attachments.
		const char *attachments = (char *)sqlite3_column_text(res, ITEM_COLUMN_ATTACHMENTS);
		if (get_largest_piece_from_item_attachments(attachments, text, &type) == false) {
			goto error;
		}
	}
	if (text->len != 0) {
		if (join_render_separator(blocks) == false) {
			goto error;
		}
		if (join_render_block(blocks, text->ptr, text->len, type) == false) {
			goto error;
		}
	}
	free_string(text);
	return true;
error:
	free_string(text);
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
	if (join_render_block(blocks, date_entry->ptr, date_entry->len, TEXT_RAW) == false) {
		free_string(date_entry);
		return false;
	}
	free_string(date_entry);
	return join_render_separator(blocks);
}

static bool
append_persons(struct render_blocks_list *blocks, sqlite3_stmt *res, const struct data_entry *entry, const char *type)
{
	const char *serialized_persons = (char *)sqlite3_column_text(res, entry->column);
	if (serialized_persons == NULL) {
		return true; // Ignore empty persons >:-D
	}
	struct string *persons = deserialize_persons_string(serialized_persons, type);
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
	if (join_render_block(blocks, block_text->ptr, block_text->len, TEXT_RAW) == false) {
		free_string(block_text);
		return false;
	}
	free_string(block_text);
	return join_render_separator(blocks);
}

static bool
append_authors(struct render_blocks_list *blocks, sqlite3_stmt *res, const struct data_entry *entry)
{
	return append_persons(blocks, res, entry, "author");
}

static bool
append_contributors(struct render_blocks_list *blocks, sqlite3_stmt *res, const struct data_entry *entry)
{
	return append_persons(blocks, res, entry, "contributor");
}

static bool
append_editors(struct render_blocks_list *blocks, sqlite3_stmt *res, const struct data_entry *entry)
{
	return append_persons(blocks, res, entry, "editor");
}

// ATTENTION! Maximal length of meta data entry name has to be reflected in MAX_METADATA_ENTRY_NAME_LENGTH.
static const struct data_entry entries[] = {
	{"feed-url",     "Feed: ",          6, ITEM_COLUMN_FEED_URL,         &append_text_line},
	{"title",        "Title: ",         7, ITEM_COLUMN_TITLE,            &append_text_line},
	{"link",         "Link: ",          6, ITEM_COLUMN_LINK,             &append_text_line},
	{"published",    "Published: ",    11, ITEM_COLUMN_PUBLICATION_DATE, &append_date},
	{"updated",      "Updated: ",       9, ITEM_COLUMN_UPDATE_DATE,      &append_date},
	{"authors",      "Authors: ",       9, ITEM_COLUMN_PERSONS,          &append_authors},
	{"contributors", "Contributors: ", 14, ITEM_COLUMN_PERSONS,          &append_contributors},
	{"editors",      "Editors: ",       9, ITEM_COLUMN_PERSONS,          &append_editors},
	{"max-content",  NULL,              0, 0,                            &append_max_content},
	{NULL,           NULL,              0, 0,                            NULL},
};

bool
generate_render_blocks_based_on_item_data(struct render_blocks_list *blocks, sqlite3_stmt *res)
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
