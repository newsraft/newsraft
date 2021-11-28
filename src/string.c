#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

struct string *
create_string(const char *src, size_t len)
{
	struct string *str = malloc(sizeof(struct string));
	if (str == NULL) {
		debug_write(DBG_FAIL, "Not enough memory for string creation!\n");
		return NULL;
	}
	/* always create a string even in cases where the src is set to NULL */
	str->ptr = malloc(sizeof(char) * (len + 1));
	if (str->ptr == NULL) {
		debug_write(DBG_FAIL, "Not enough memory for string creation!\n");
		free(str);
		return NULL;
	}
	if (src == NULL) {
		str->len = 0;
		str->lim = len;
	} else {
		if (len != 0) {
			memcpy(str->ptr, src, sizeof(char) * len);
		}
		str->len = len;
		str->lim = len;
	}
	*(str->ptr + len) = '\0';
	return str;
}

struct string *
create_empty_string(void)
{
	return create_string(NULL, 0);
}

void
cpy_string_array(struct string *dest, const char *src_ptr, size_t src_len)
{
	dest->len = src_len;
	if (dest->len > dest->lim) {
		dest->lim = dest->len;
		dest->ptr = realloc(dest->ptr, dest->lim + 1);
	}
	strcpy(dest->ptr, src_ptr);
}

void
cpy_string_string(struct string *dest, const struct string *src)
{
	cpy_string_array(dest, src->ptr, src->len);
}

void
cat_string_array(struct string *dest, const char *src_ptr, size_t src_len)
{
	dest->len += src_len;
	if (dest->len > dest->lim) {
		dest->lim = dest->len;
		dest->ptr = realloc(dest->ptr, dest->lim + 1);
	}
	strncat(dest->ptr, src_ptr, src_len);
}

void
cat_string_string(struct string *dest, const struct string *src)
{
	cat_string_array(dest, src->ptr, src->len);
}

void
cat_string_char(struct string *dest, char c)
{
	dest->len += 1;
	if (dest->len > dest->lim) {
		dest->lim = dest->len;
		dest->ptr = realloc(dest->ptr, dest->lim + 1);
	}
	*(dest->ptr + dest->len - 1) = c;
	*(dest->ptr + dest->len) = '\0';
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
strip_whitespace_from_edges(char *str, size_t *len)
{
	if (*len == 0) {
		return;
	}

	size_t left_edge = 0, right_edge = *len - 1;
	while ((*(str + left_edge) == ' '   ||
	        *(str + left_edge) == '\t'  ||
	        *(str + left_edge) == '\r'  ||
	        *(str + left_edge) == '\n') &&
	       left_edge <= right_edge)
	{
		++left_edge;
	}
	while ((*(str + right_edge) == ' '   ||
	        *(str + right_edge) == '\t'  ||
	        *(str + right_edge) == '\r'  ||
	        *(str + right_edge) == '\n') &&
	       right_edge >= left_edge)
	{
		--right_edge;
	}

	if ((left_edge == 0) && (right_edge == (*len - 1))) {
		return;
	}

	if (right_edge < left_edge) {
		*(str + 0) = '\0';
		*len = 0;
		return;
	}

	size_t stripped_string_len = right_edge - left_edge + 1;
	for (size_t i = 0; i < stripped_string_len; ++i) {
		*(str + i) = *(str + i + left_edge);
	}
	*len = stripped_string_len;
	*(str + stripped_string_len) = '\0';
}
