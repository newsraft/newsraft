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

static const char caret[2] = {DELIMITER, '^'};

bool
serialize_caret(struct string **target)
{
	if (*target == NULL) {
		*target = crtes(100);
	}
	return *target != NULL ? catas(*target, caret, 2) : false;
}

bool
serialize_array(register struct string **target, register const char *key, register size_t key_len, register const char *value, register size_t value_len)
{
	if (value == NULL || value_len == 0) {
		return true; // Ignore empty entries.
	}
	if (make_string_fit_more(target, 1 + key_len + value_len) == false) {
		return false;
	}

	// Check tests/serialize_array.c to better understand what this function does.

	// Just append key and value to the target with DELIMITER at the beginning.
	// It's kind of messy here because we need to squeeze out all available performance
	// since this function does a lot of heavy lifting in the feed parsers code.
	(*target)->ptr[(*target)->len] = DELIMITER;
	memcpy((*target)->ptr + (*target)->len + 1, key, key_len);
	memcpy((*target)->ptr + (*target)->len + 1 + key_len, value, value_len);
	(*target)->len += 1 + key_len + value_len;
	(*target)->ptr[(*target)->len] = '\0';

	// Make sure there's no delimiters in the serialized data!
	for (size_t i = 0, end = (*target)->len; i < value_len; ++i) {
		if ((*target)->ptr[end - i - 1] == DELIMITER) {
			for (char *j = (*target)->ptr + end - i - 1; *j != '\0'; ++j) {
				*j = *(j + 1);
			}
			(*target)->len -= 1;
			(*target)->ptr[(*target)->len] = '\0';
			if ((*target)->len + value_len == end) {
				(*target)->len -= 1 + key_len;
				(*target)->ptr[(*target)->len] = '\0';
				return true; // Value consists only of delimiters!
			}
		}
	}
	return true;
}

bool
serialize_string(struct string **target, const char *key, size_t key_len, const struct string *value)
{
	return value != NULL && value->len > 0 ? serialize_array(target, key, key_len, value->ptr, value->len) : true;
}

struct deserialize_stream *
open_deserialize_stream(const char *serialized_data)
{
	struct deserialize_stream *stream = malloc(sizeof(struct deserialize_stream));
	if (stream == NULL) {
		return NULL;
	}
	stream->entry = crtes(10000);
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
		if (cpyas(&stream->entry, stream->prev_delimiter_pos + 1, strlen(stream->prev_delimiter_pos + 1)) == false) {
			return NULL;
		}
		stream->prev_delimiter_pos = NULL;
	} else {
		if (cpyas(&stream->entry, stream->prev_delimiter_pos + 1, stream->delimiter_pos - stream->prev_delimiter_pos - 1) == false) {
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
