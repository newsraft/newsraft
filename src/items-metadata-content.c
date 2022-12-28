#include <string.h>
#include "newsraft.h"

static inline render_block_format
get_content_type_by_string(const char *type)
{
	return (strstr(type, "html") != NULL) || (strstr(type, "HTML") != NULL) ? TEXT_HTML : TEXT_PLAIN;
}

static bool
get_largest_text_piece_from_item_serialized_data(
	const char *data,
	struct string *text,
	render_block_format *type,
	const char *text_prefix,
	size_t text_prefix_len,
	const char *type_prefix,
	size_t type_prefix_len)
{
	if (data == NULL) {
		return true; // There's no data. Ignore it.
	}
	struct string *temp_text = crtes();
	struct deserialize_stream *stream = open_deserialize_stream(data);
	if ((temp_text == NULL) || (stream == NULL)) {
		goto error;
	}
	render_block_format temp_type = TEXT_PLAIN;
	const struct string *entry = get_next_entry_from_deserialize_stream(stream);
	while (entry != NULL) {
		if (strcmp(entry->ptr, "^") == 0) {
			if (temp_text->len > text->len) {
				if (cpyss(text, temp_text) == false) {
					goto error;
				}
				*type = temp_type;
			}
			empty_string(temp_text);
			temp_type = TEXT_PLAIN;
		} else if (strncmp(entry->ptr, type_prefix, type_prefix_len) == 0) {
			temp_type = get_content_type_by_string(entry->ptr + type_prefix_len);
		} else if (strncmp(entry->ptr, text_prefix, text_prefix_len) == 0) {
			if (cpyas(temp_text, entry->ptr + text_prefix_len, entry->len - text_prefix_len) == false) {
				goto error;
			}
		}
		entry = get_next_entry_from_deserialize_stream(stream);
	}
	if (temp_text->len > text->len) {
		if (cpyss(text, temp_text) == false) {
			goto error;
		}
		*type = temp_type;
	}
	free_string(temp_text);
	close_deserialize_stream(stream);
	return true;
error:
	free_string(temp_text);
	close_deserialize_stream(stream);
	return false;
}

bool
get_largest_piece_from_item_content(const char *content, struct string *text, render_block_format *type)
{
	return get_largest_text_piece_from_item_serialized_data(content, text, type, "text=", 5, "type=", 5);
}

bool
get_largest_piece_from_item_attachments(const char *attachments, struct string *text, render_block_format *type)
{
	return get_largest_text_piece_from_item_serialized_data(attachments, text, type, "description_text=", 17, "description_type=", 17);
}
