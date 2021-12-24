#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

struct string *
create_string(const char *src, size_t len)
{
	struct string *str = malloc(sizeof(struct string));
	if (str == NULL) {
		FAIL("Not enough memory for string structure!");
		return NULL;
	}
	str->ptr = malloc(sizeof(char) * (len + 1));
	if (str->ptr == NULL) {
		FAIL("Not enough memory for string pointer!");
		free(str);
		return NULL;
	}
	if (src != NULL) {
		if (len != 0) {
			memcpy(str->ptr, src, sizeof(char) * len);
		}
		str->len = len;
	} else {
		str->len = 0;
	}
	str->lim = len;
	*(str->ptr + str->len) = '\0';
	return str;
}

struct string *
create_empty_string(void)
{
	return create_string(NULL, 0);
}

// Copy array to string.
// On success returns 0.
// On failure returns non-zero.
int
cpyas(struct string *dest, const char *src_ptr, size_t src_len)
{
	if (src_len > dest->lim) {
		// Multiply by 2 to decrease number of further realloc calls.
		char *temp = realloc(dest->ptr, sizeof(char) * (src_len * 2 + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for copying array to string!");
			return 1;
		}
		dest->ptr = temp;
		dest->lim = src_len * 2;
	}
	memcpy(dest->ptr, src_ptr, sizeof(char) * src_len);
	*(dest->ptr + src_len) = '\0';
	dest->len = src_len;
	return 0;
}

// Copy string to string.
int
cpyss(struct string *dest, const struct string *src)
{
	return cpyas(dest, src->ptr, src->len);
}

// Concatenate array to string.
// On success returns 0.
// On failure returns non-zero.
int
catas(struct string *dest, const char *src_ptr, size_t src_len)
{
	size_t new_len = dest->len + src_len;
	if (new_len > dest->lim) {
		// Multiply by 2 to decrease number of further realloc calls.
		char *temp = realloc(dest->ptr, sizeof(char) * (new_len * 2 + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for concatenating array to string!");
			return 1;
		}
		dest->ptr = temp;
		dest->lim = new_len * 2;
	}
	strncat(dest->ptr, src_ptr, src_len);
	*(dest->ptr + new_len) = '\0';
	dest->len = new_len;
	return 0;
}

// Concatenate string to string.
int
catss(struct string *dest, const struct string *src)
{
	return catas(dest, src->ptr, src->len);
}

// Concatenate character to string.
// On success returns 0.
// On failure returns non-zero.
int
catcs(struct string *dest, char c)
{
	size_t new_len = dest->len + 1;
	if (new_len > dest->lim) {
		// Multiply by 2 to decrease number of further realloc calls.
		char *temp = realloc(dest->ptr, sizeof(char) * (new_len * 2 + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for concatenating character to string!");
			return 1;
		}
		dest->ptr = temp;
		dest->lim = new_len * 2;
	}
	*(dest->ptr + dest->len) = c;
	dest->len = new_len;
	*(dest->ptr + dest->len) = '\0';
	return 0;
}

void
empty_string(struct string *str)
{
	str->len = 0;
	*(str->ptr + 0) = '\0';
}

void
free_string(struct string *str)
{
	if (str == NULL) {
		return;
	}
	free(str->ptr);
	free(str);
}

void
strip_whitespace_from_edges(struct string *str)
{
	if (str->len == 0) {
		return;
	}

	size_t left_edge = 0, right_edge = str->len - 1;
	while ((*(str->ptr + left_edge) == ' '   ||
	        *(str->ptr + left_edge) == '\t'  ||
	        *(str->ptr + left_edge) == '\r'  ||
	        *(str->ptr + left_edge) == '\n') &&
	       left_edge <= right_edge)
	{
		++left_edge;
	}
	while ((*(str->ptr + right_edge) == ' '   ||
	        *(str->ptr + right_edge) == '\t'  ||
	        *(str->ptr + right_edge) == '\r'  ||
	        *(str->ptr + right_edge) == '\n') &&
	       right_edge >= left_edge)
	{
		--right_edge;
	}

	if ((left_edge == 0) && (right_edge == (str->len - 1))) {
		return;
	}

	if (right_edge < left_edge) {
		*(str->ptr + 0) = '\0';
		str->len = 0;
		return;
	}

	size_t stripped_string_len = right_edge - left_edge + 1;
	for (size_t i = 0; i < stripped_string_len; ++i) {
		*(str->ptr + i) = *(str->ptr + i + left_edge);
	}
	str->len = stripped_string_len;
	*(str->ptr + stripped_string_len) = '\0';
}
