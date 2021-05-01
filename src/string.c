#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

struct string *
create_string(void)
{
	struct string *str = malloc(sizeof(struct string));
	if (str == NULL) return NULL;
	str->len = 0;
	str->ptr = malloc(sizeof(char));
	if (str->ptr == NULL) { free(str); return NULL; }
	*(str->ptr + 0) = '\0';
	return str;
}

void
make_string(struct string **dest, void *src, size_t len)
{
	if (*dest == NULL) *dest = create_string();
	if (*dest == NULL) return;
	(*dest)->ptr = realloc((*dest)->ptr, sizeof(char) * (len + 1));
	if ((*dest)->ptr == NULL) { free_string(dest); return; }
	(*dest)->len = len;
	memcpy((*dest)->ptr, src, sizeof(char) * len);
	*((*dest)->ptr + len) = '\0';
	if ((*dest)->ptr == NULL) free_string(dest);
}

void
free_string(struct string **dest)
{
	if (*dest == NULL) return;
	if ((*dest)->ptr != NULL) free((*dest)->ptr);
	free(*dest);
	*dest = NULL;
}

void
cat_string_string(struct string *dest, struct string *src)
{
	if (src->len == 0) return;
	dest->len += src->len;
	dest->ptr = realloc(dest->ptr, dest->len + 1);
	strcat(dest->ptr, src->ptr);
}

void
cat_string_array(struct string *dest, char *src)
{
	size_t src_len = strlen(src);
	if (src_len == 0) return;
	dest->len += src_len;
	dest->ptr = realloc(dest->ptr, (dest->len + 1));
	strcat(dest->ptr, src);
}

void
cat_string_char(struct string *dest, char c)
{
	++(dest->len);
	dest->ptr = realloc(dest->ptr, sizeof(char) * (dest->len + 1));
	*(dest->ptr + dest->len - 1) = c;
	*(dest->ptr + dest->len) = '\0';
}
