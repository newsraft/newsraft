#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

struct deserialize_stream {
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

static const char caret[2] = {DELIMITER, '^'};

static inline void
remove_delimiter_from_string_with_start_offset(struct string *str, size_t offset)
{
	for (char *i = str->ptr + offset; *i != '\0'; ++i) {
		if (*i == DELIMITER) {
			for (char *j = i; *j != '\0'; ++j) {
				*j = *(j + 1);
			}
			str->len -= 1;
			i -= 1;
		}
	}
}

bool
serialize_caret(struct string **target)
{
	if (*target != NULL) {
		return catas(*target, caret, 2);
	}
	*target = crtes(100);
	return *target != NULL ? catas(*target, caret, 2) : false;
}

bool
serialize_array(struct string **target, const char *key, size_t key_len, const char *value, size_t value_len)
{
	if ((value == NULL) || (value_len == 0)) {
		return true; // Ignore empty entries.
	}
	if (*target == NULL) {
		*target = crtes(100);
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
	size_t old_len = (*target)->len;
	if (catas(*target, value, value_len) == false) {
		return false;
	}
	remove_delimiter_from_string_with_start_offset(*target, old_len);
	return true;
}

bool
serialize_string(struct string **target, const char *key, size_t key_len, struct string *value)
{
	if ((value == NULL) || (value->len == 0)) {
		return true; // Ignore empty entries.
	}
	return serialize_array(target, key, key_len, value->ptr, value->len);
}

struct deserialize_stream *
open_deserialize_stream(const char *serialized_data)
{
	struct deserialize_stream *stream = malloc(sizeof(struct deserialize_stream));
	if (stream == NULL) {
		return NULL;
	}
	stream->entry = crtes(100);
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
get_next_entry_from_deserialize_stream(struct deserialize_stream *stream)
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
close_deserialize_stream(struct deserialize_stream *stream)
{
	if (stream != NULL) {
		free_string(stream->entry);
		free(stream);
	}
}
