#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

struct string_deserialize_stream {
	struct string *entry;
	char delimiter;
	const char *prev_delimiter_pos;
	const char *delimiter_pos;
};

// You can change it without breaking compatibility with existing databases,
// because first character of serialized data is always a DELIMITER. And it
// is easy for the stream handler (see below) to figure out which character
// to take as a delimiter.
#define DELIMITER 31 // ASCII Unit Separator (control character)

// However, this constant must never be changed. Or wicked times will come!
#define SEPARATOR '='

bool
cat_array_to_serialization(struct string **target, const char *key, size_t key_len, const char *value, size_t value_len)
{
	if ((value == NULL) || (value_len == 0)) {
		return true; // Ignore empty entries.
	}
	if (*target == NULL) {
		*target = crtes();
		if (*target == NULL) {
			return false;
		}
	}
	if (catcs(*target, DELIMITER) == false) {
		return false;
	}
	if (catas(*target, key, key_len) == false) {
		return false;
	}
	if (catcs(*target, SEPARATOR) == false) {
		return false;
	}
	struct string *str = crtas(value, value_len);
	if (str == NULL) {
		return false;
	}
	remove_character_from_string(str, DELIMITER);
	if (catss(*target, str) == false) {
		free_string(str);
		return false;
	}
	free_string(str);
	return true;
}

bool
cat_string_to_serialization(struct string **target, const char *key, size_t key_len, struct string *value)
{
	if ((value == NULL) || (value->len == 0)) {
		return true; // Ignore empty entries.
	}
	if (*target == NULL) {
		*target = crtes();
		if (*target == NULL) {
			return false;
		}
	}
	if (catcs(*target, DELIMITER) == false) {
		return false;
	}
	if (catas(*target, key, key_len) == false) {
		return false;
	}
	if (catcs(*target, SEPARATOR) == false) {
		return false;
	}
	remove_character_from_string(value, DELIMITER);
	if (catss(*target, value) == false) {
		return false;
	}
	return true;
}

bool
cat_caret_to_serialization(struct string **target)
{
	if (*target == NULL) {
		*target = crtes();
		if (*target == NULL) {
			return false;
		}
	}
	if (catcs(*target, DELIMITER) == false) {
		return false;
	}
	return catcs(*target, '^');
}

struct string_deserialize_stream *
open_string_deserialize_stream(const char *serialized_data)
{
	struct string_deserialize_stream *stream = malloc(sizeof(struct string_deserialize_stream));
	if (stream == NULL) {
		return NULL;
	}
	stream->entry = crtes();
	if (stream->entry == NULL) {
		free(stream);
		return NULL;
	}
	stream->delimiter = *serialized_data;
	if (stream->delimiter == '\0') {
		stream->prev_delimiter_pos = NULL;
		stream->delimiter_pos = NULL;
	} else {
		stream->prev_delimiter_pos = serialized_data;
		stream->delimiter_pos = strchr(serialized_data + 1, stream->delimiter);
	}
	return stream;
}

const struct string *
get_next_entry_from_deserialize_stream(struct string_deserialize_stream *stream)
{
	if ((stream->delimiter_pos == NULL) && (stream->prev_delimiter_pos == NULL)) {
		return NULL;
	}
	if (stream->delimiter_pos == NULL) {
		if (cpyas(stream->entry, stream->prev_delimiter_pos + 1, strlen(stream->prev_delimiter_pos + 1)) == false) {
			return NULL;
		}
		stream->prev_delimiter_pos = NULL;
	} else {
		if (cpyas(stream->entry, stream->prev_delimiter_pos + 1, stream->delimiter_pos - stream->prev_delimiter_pos - 1) == false) {
			return NULL;
		}
		stream->prev_delimiter_pos = stream->delimiter_pos;
		stream->delimiter_pos = strchr(stream->delimiter_pos + 1, stream->delimiter);
	}
	return stream->entry;
}

void
close_string_deserialize_stream(struct string_deserialize_stream *stream)
{
	free_string(stream->entry);
	free(stream);
}
