#include <string.h>
#include "newsraft.h"

#define MAX_METADATA_ENTRY_NAME_LENGTH 12 // Currently it is "contributors".

struct data_entry {
	const char *const field;   // Name of field to match in config_contents_meta_data.
	const char *const tooltip; // String to write before entry data.
	const size_t tooltip_len;
	bool (*append_handler)(struct render_blocks_list *blocks, const struct item_entry *item, sqlite3_stmt *res, const struct data_entry *entry);
};

static bool
append_feed_line(struct render_blocks_list *blocks, const struct item_entry *item, sqlite3_stmt *res, const struct data_entry *entry)
{
	(void)res;
	struct string *data = crtas(entry->tooltip, entry->tooltip_len);
	if (data == NULL) {
		return false;
	}
	if ((item->feed->name != NULL) && (item->feed->name->len != 0)) {
		if (catss(data, item->feed->name) == false) {
			goto error;
		}
	} else {
		if (catss(data, item->feed->link) == false) {
			goto error;
		}
	}
	inlinefy_string(data);
	if (join_render_block(blocks, data->ptr, data->len, TEXT_RAW) == false) {
		goto error;
	}
	free_string(data);
	return true;
error:
	free_string(data);
	return false;
}

static bool
append_line(struct render_blocks_list *blocks, const struct item_entry *item, sqlite3_stmt *res, const struct data_entry *entry)
{
	(void)res;
	const struct string *text = entry->tooltip[0] == 'T' ? item->title : item->url;
	if ((text == NULL) || (text->len == 0)) {
		return true;
	}
	struct string *data = crtas(entry->tooltip, entry->tooltip_len);
	if (data != NULL) {
		if (catss(data, text) == true) {
			inlinefy_string(data);
			if (join_render_block(blocks, data->ptr, data->len, TEXT_RAW) == true) {
				free_string(data);
				return true;
			}
		}
		free_string(data);
	}
	return false;
}

static bool
append_date(struct render_blocks_list *blocks, const struct item_entry *item, sqlite3_stmt *res, const struct data_entry *entry)
{
	(void)res;
	int64_t date = entry->tooltip[0] == 'P' ? item->pub_date : item->upd_date;
	if (date == 0) {
		return true; // It is not an error because this item simply does not have this date set.
	}
	struct string *date_str = get_config_date_str(date, CFG_CONTENT_DATE_FORMAT);
	if (date_str != NULL) {
		struct string *date_entry = crtas(entry->tooltip, entry->tooltip_len);
		if (date_entry != NULL) {
			if (catss(date_entry, date_str) == true) {
				if (join_render_block(blocks, date_entry->ptr, date_entry->len, TEXT_RAW) == true) {
					free_string(date_str);
					free_string(date_entry);
					return true;
				}
			}
			free_string(date_entry);
		}
		free_string(date_str);
	}
	return false;
}

static bool
append_persons(struct render_blocks_list *blocks, const struct item_entry *item, sqlite3_stmt *res, const struct data_entry *entry)
{
	(void)item;
	const char *serialized_persons = (char *)sqlite3_column_text(res, ITEM_COLUMN_PERSONS);
	if (serialized_persons == NULL) {
		return true; // Ignore empty persons >:-D
	}
	char type[100]; // Entry field name without the last letter.
	strcpy(type, entry->field);
	type[strlen(entry->field) - 1] = '\0';
	struct string *persons = deserialize_persons_string(serialized_persons, type);
	if ((persons == NULL) || (persons->len == 0)) {
		free_string(persons);
		return true; // There is nothing here that we were looking for.
	}
	struct string *block_text = crtas(entry->tooltip, entry->tooltip_len);
	if (block_text != NULL) {
		if (catss(block_text, persons) == true) {
			if (join_render_block(blocks, block_text->ptr, block_text->len, TEXT_RAW) == true) {
				free_string(persons);
				free_string(block_text);
				return true;
			}
		}
		free_string(block_text);
	}
	free_string(persons);
	return false;
}

static bool
append_max_content(struct render_blocks_list *blocks, const struct item_entry *item, sqlite3_stmt *res, const struct data_entry *entry)
{
	(void)item;
	(void)entry;
	const char *content = (char *)sqlite3_column_text(res, ITEM_COLUMN_CONTENT);
	struct string *text = crtes(50000);
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
		join_render_separator(blocks);
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

// ATTENTION! Maximal length of meta data entry name has to be reflected in MAX_METADATA_ENTRY_NAME_LENGTH.
static const struct data_entry entries[] = {
	{"feed",         "Feed: ",          6, &append_feed_line},
	{"title",        "Title: ",         7, &append_line},
	{"link",         "Link: ",          6, &append_line},
	{"published",    "Published: ",    11, &append_date},
	{"updated",      "Updated: ",       9, &append_date},
	{"authors",      "Authors: ",       9, &append_persons},
	{"contributors", "Contributors: ", 14, &append_persons},
	{"editors",      "Editors: ",       9, &append_persons},
	{"max-content",  NULL,              0, &append_max_content},
	{NULL,           NULL,              0, NULL},
};

bool
generate_render_blocks_based_on_item_data(struct render_blocks_list *blocks, const struct item_entry *item, sqlite3_stmt *res)
{
	char entry[MAX_METADATA_ENTRY_NAME_LENGTH + 1];
	size_t entry_len = 0;
	const struct string *content_order = get_cfg_string(CFG_ITEM_FORMATION_ORDER);
	const char *i = content_order->ptr;
	while (true) {
		if ((*i == ',') || (*i == '\0')) {
			for (size_t j = 0; entries[j].field != NULL; ++j) {
				if (strncmp(entry, entries[j].field, entry_len) == 0) {
					if (entries[j].append_handler(blocks, item, res, entries + j) == false) {
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
