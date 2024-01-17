#include <string.h>
#include "newsraft.h"

static struct string *
block_str(const struct string *text)
{
	if (text != NULL && text->len > 0) {
		struct string *data = crtss(text);
		if (data != NULL) {
			inlinefy_string(data);
			return data;
		}
	}
	return NULL;
}

static struct string *
block_date(const struct item_entry *item)
{
	if (item->pub_date == 0 && item->upd_date == 0) {
		return NULL;
	}
	struct string *date_entry = crtes(100);
	struct string *date_str = get_config_date_str(item->pub_date == 0 ? item->upd_date : item->pub_date, CFG_ITEM_CONTENT_DATE_FORMAT);
	if (date_entry == NULL || date_str == NULL) goto error;
	if (item->pub_date > 0 && item->upd_date > 0 && item->pub_date != item->upd_date) {
		if (catss(date_entry, date_str) == false) goto error;
		if (catas(date_entry, " (updated ", 10) == false) goto error;
		free_string(date_str);
		date_str = get_config_date_str(item->upd_date, CFG_ITEM_CONTENT_DATE_FORMAT);
		if (date_str == NULL) goto error;
		if (catcs(date_str, ')') == false) goto error;
	}
	if (catss(date_entry, date_str) == false) goto error;
	struct string *data = block_str(date_entry);
	free_string(date_entry);
	free_string(date_str);
	return data;
error:
	free_string(date_entry);
	free_string(date_str);
	return NULL;
}

static struct string *
block_persons(sqlite3_stmt *res)
{
	const char *serialized_persons = (char *)sqlite3_column_text(res, ITEM_COLUMN_PERSONS);
	if (serialized_persons == NULL) return NULL;
	struct string *persons = deserialize_persons_string(serialized_persons);
	struct string *data = block_str(persons);
	free_string(persons);
	return data;
}

static struct string *
block_max_content(sqlite3_stmt *res, render_block_format *output_type)
{
	const char *content = (char *)sqlite3_column_text(res, ITEM_COLUMN_CONTENT);
	struct string *text = crtes(50000);
	if (text == NULL) {
		return NULL;
	}
	render_block_format type = TEXT_PLAIN;
	if (get_largest_piece_from_item_content(content, &text, &type) == false) {
		goto error;
	}
	if (text->len == 0) {
		// There were no texts in the content, let's try to search in
		// the descriptions for item's attachments.
		const char *attachments = (char *)sqlite3_column_text(res, ITEM_COLUMN_ATTACHMENTS);
		if (get_largest_piece_from_item_attachments(attachments, &text, &type) == false) {
			goto error;
		}
	}
	if (text->len > 0) {
		*output_type = type;
		return text;
	}
error:
	free_string(text);
	return NULL;
}

bool
generate_render_blocks_based_on_item_data(struct render_blocks_list *blocks, const struct item_entry *item, sqlite3_stmt *res)
{
#define MAX_ENTRY_LENGTH 1000
	char entry[MAX_ENTRY_LENGTH + 10];
	size_t entry_len = 0;
	const struct string *content_order = get_cfg_string(CFG_ITEM_CONTENT_FORMAT);
	for (const char *i = content_order->ptr; ; ++i) {
		if (*i == '|' || *i == '\0') {
			entry[entry_len] = '\0';
			char *percent_pos = strchr(entry, '%');
			if (percent_pos == NULL) {
				add_render_block(blocks, entry, entry_len, TEXT_HTML, false);
			} else if (*(percent_pos + 1) != '\0') {
				*percent_pos = '\0';
				char specifier = *(percent_pos + 1);
				if (specifier == 'L') {
					// Block with links list will be added later, in apply_links_render_blocks call.
					// It's this way because links list content depends on what item content has.
					add_render_block(blocks, entry, strlen(entry), TEXT_HTML, false);
					blocks->links_block_index = blocks->len;
					add_render_block(blocks, percent_pos + 2, strlen(percent_pos + 2), TEXT_HTML, false);
				} else if (specifier == 'c') {
					render_block_format type = TEXT_PLAIN;
					struct string *content = block_max_content(res, &type);
					if (content != NULL) {
						add_render_block(blocks, entry, strlen(entry), TEXT_HTML, false);
						add_render_block(blocks, content->ptr, content->len, type, true);
						add_render_block(blocks, percent_pos + 2, strlen(percent_pos + 2), TEXT_HTML, false);
						free_string(content);
					}
				} else {
					struct string *value = NULL;
					switch (specifier) {
						case 'f': value = block_str(item->feed->name != NULL && item->feed->name->len != 0 ? item->feed->name : item->feed->link); break;
						case 't': value = block_str(item->title); break;
						case 'l': value = block_str(item->url);   break;
						case 'd': value = block_date(item);       break;
						case 'a': value = block_persons(res);     break;
					}
					if (value != NULL) {
						struct string *text = crtas(entry, strlen(entry));
						catss(text, value);
						catas(text, percent_pos + 2, strlen(percent_pos + 2));
						add_render_block(blocks, text->ptr, text->len, TEXT_HTML, false);
						free_string(value);
						free_string(text);
					}
				}
			}
			if (*i == '\0') break;
			entry_len = 0;
		} else if (entry_len < MAX_ENTRY_LENGTH) {
			entry[entry_len++] = *i;
		}
	}
	return true;
}
