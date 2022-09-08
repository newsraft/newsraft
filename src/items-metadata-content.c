#include <string.h>
#include "newsraft.h"

static bool
get_largest_text_piece_from_item_serialized_data(
	const char *data,
	struct string *text,
	struct string *type,
	const char *text_prefix,
	size_t text_prefix_len,
	const char *type_prefix,
	size_t type_prefix_len)
{
	if (data == NULL) {
		return true; // There's no data. Ignore it.
	}
	struct string *temp_text = crtes();
	if (temp_text == NULL) {
		goto undo0;
	}
	struct string *temp_type = crtas("text/plain", 10);
	if (temp_type == NULL) {
		goto undo1;
	}
	struct deserialize_stream *stream = open_deserialize_stream(data);
	if (stream == NULL) {
		goto undo2;
	}
	const struct string *entry = get_next_entry_from_deserialize_stream(stream);
	while (entry != NULL) {
		if (strcmp(entry->ptr, "^") == 0) {
			if (temp_text->len > text->len) {
				if (cpyss(text, temp_text) == false) {
					goto undo3;
				}
				if (cpyss(type, temp_type) == false) {
					goto undo3;
				}
			}
			empty_string(temp_text);
			if (cpyas(temp_type, "text/plain", 10) == false) {
				goto undo3;
			}
		} else if (strncmp(entry->ptr, type_prefix, type_prefix_len) == 0) {
			if (cpyas(temp_type, entry->ptr + type_prefix_len, entry->len - type_prefix_len) == false) {
				goto undo3;
			}
		} else if (strncmp(entry->ptr, text_prefix, text_prefix_len) == 0) {
			if (cpyas(temp_text, entry->ptr + text_prefix_len, entry->len - text_prefix_len) == false) {
				goto undo3;
			}
		}
		entry = get_next_entry_from_deserialize_stream(stream);
	}
	if (temp_text->len > text->len) {
		if (cpyss(text, temp_text) == false) {
			goto undo3;
		}
		if (cpyss(type, temp_type) == false) {
			goto undo3;
		}
	}
	free_string(temp_text);
	free_string(temp_type);
	close_deserialize_stream(stream);
	return true;
undo3:
	close_deserialize_stream(stream);
undo2:
	free_string(temp_type);
undo1:
	free_string(temp_text);
undo0:
	return false;
}

bool
get_largest_piece_from_item_content(const char *content, struct string *text, struct string *type)
{
	return get_largest_text_piece_from_item_serialized_data(content, text, type, "text=", 5, "type=", 5);
}

bool
get_largest_piece_from_item_attachments(const char *attachments, struct string *text, struct string *type)
{
	return get_largest_text_piece_from_item_serialized_data(attachments, text, type, "description_text=", 17, "description_type=", 17);
}
