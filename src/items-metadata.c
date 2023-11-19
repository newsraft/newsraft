#include <string.h>
#include "newsraft.h"

static struct render_block *
block_str(const struct string *text)
{
	if (text != NULL && text->len > 0) {
		struct string *data = crtss(text);
		if (data != NULL) {
			inlinefy_string(data);
			struct render_block *block = create_render_block(data->ptr, data->len, TEXT_RAW, true);
			free_string(data);
			return block;
		}
	}
	return NULL;
}

static struct render_block *
block_date(const struct item_entry *item)
{
	if (item->pub_date == 0 && item->upd_date == 0) {
		return NULL;
	}
	struct string *date_entry = crtes(100);
	struct string *date_str = get_config_date_str(item->pub_date == 0 ? item->upd_date : item->pub_date, CFG_CONTENT_DATE_FORMAT);
	if (date_entry == NULL || date_str == NULL) goto error;
	if (item->pub_date > 0 && item->upd_date > 0 && item->pub_date != item->upd_date) {
		if (catss(date_entry, date_str) == false) goto error;
		if (catas(date_entry, " (updated ", 10) == false) goto error;
		free_string(date_str);
		date_str = get_config_date_str(item->upd_date, CFG_CONTENT_DATE_FORMAT);
		if (date_str == NULL) goto error;
		if (catcs(date_str, ')') == false) goto error;
	}
	if (catss(date_entry, date_str) == false) goto error;
	struct render_block *block = block_str(date_entry);
	free_string(date_entry);
	free_string(date_str);
	return block;
error:
	free_string(date_entry);
	free_string(date_str);
	return NULL;
}

static struct render_block *
block_persons(sqlite3_stmt *res)
{
	const char *serialized_persons = (char *)sqlite3_column_text(res, ITEM_COLUMN_PERSONS);
	if (serialized_persons == NULL) return NULL;
	struct string *persons = deserialize_persons_string(serialized_persons);
	struct render_block *block = block_str(persons);
	free_string(persons);
	return block;
}

static struct render_block *
block_max_content(sqlite3_stmt *res)
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
		struct render_block *block = create_render_block(text->ptr, text->len, type, true);
		free_string(text);
		return block;
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
				struct render_block *simple_block = create_render_block(entry, entry_len, TEXT_HTML, false);
				join_render_block(blocks, simple_block);
				free_render_block(simple_block);
			} else {
				const char *specifier_pos = percent_pos + 1;
				char specifier = *specifier_pos;
				*percent_pos = '\0';
				struct render_block *block1 = create_render_block(entry, strlen(entry), TEXT_HTML, false);
				struct render_block *block2 = NULL;
				struct render_block *block3 = create_render_block(specifier_pos + 1, strlen(specifier_pos + 1), TEXT_HTML, false);
				if (specifier == 'L') {
					// Blocks related to links list will be added later, in apply_links_render_blocks call.
					// It's this way because links list content depends on what item content has.
					blocks->links_block_index = blocks->len;
					blocks->pre_links_block = block1;
					blocks->post_links_block = block3;
				} else {
					switch (specifier) {
						case 'f': block2 = block_str(item->feed->name != NULL && item->feed->name->len != 0 ? item->feed->name : item->feed->link); break;
						case 't': block2 = block_str(item->title); break;
						case 'l': block2 = block_str(item->url);   break;
						case 'd': block2 = block_date(item);       break;
						case 'a': block2 = block_persons(res);     break;
						case 'c': block2 = block_max_content(res); break;
					}
					if (block2 != NULL) {
						join_render_block(blocks, block1);
						join_render_block(blocks, block2);
						join_render_block(blocks, block3);
					}
					free_render_block(block1);
					free_render_block(block2);
					free_render_block(block3);
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
