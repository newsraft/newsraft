#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

struct string *
create_string(char *src, size_t len)
{
	struct string *str = malloc(sizeof(struct string));
	if (str == NULL) {
		return NULL;
	}
	/* always create a string even in cases where the src is set to NULL */
	str->ptr = malloc(sizeof(char) * (len + 1));
	if (str->ptr == NULL) {
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
trim_string(struct string *dest)
{
	if (dest->len != dest->lim) {
		dest->ptr = realloc(dest->ptr, (dest->len + 1));
		dest->lim = dest->len;
	}
}

void
free_string(struct string *str)
{
	if (str != NULL) {
		free(str->ptr);
		free(str);
	}
}

void
cpy_string_string(struct string *dest, struct string *src)
{
	dest->len = src->len;
	if (dest->len > dest->lim) {
		dest->lim = dest->len;
		dest->ptr = realloc(dest->ptr, dest->lim + 1);
	}
	strcpy(dest->ptr, src->ptr);
}

void
cpy_string_array(struct string *dest, char *src_ptr, size_t src_len)
{
	dest->len = src_len;
	if (dest->len > dest->lim) {
		dest->lim = dest->len;
		dest->ptr = realloc(dest->ptr, dest->lim + 1);
	}
	strcpy(dest->ptr, src_ptr);
}

void
cpy_string_char(struct string *dest, char c)
{
	dest->len = 1;
	if (dest->len > dest->lim) {
		dest->lim = dest->len;
		dest->ptr = realloc(dest->ptr, dest->lim + 1);
	}
	*(dest->ptr + 0) = c;
	*(dest->ptr + 1) = '\0';
}

void
cat_string_string(struct string *dest, struct string *src)
{
	dest->len += src->len;
	if (dest->len > dest->lim) {
		dest->lim = dest->len;
		dest->ptr = realloc(dest->ptr, dest->lim + 1);
	}
	strncat(dest->ptr, src->ptr, src->len);
}

void
cat_string_array(struct string *dest, char *src_ptr, size_t src_len)
{
	dest->len += src_len;
	if (dest->len > dest->lim) {
		dest->lim = dest->len;
		dest->ptr = realloc(dest->ptr, dest->lim + 1);
	}
	strncat(dest->ptr, src_ptr, src_len);
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
make_string_empty(struct string *str)
{
	str->len = 0;
	*(str->ptr + 0) = '\0';
}
