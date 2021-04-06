#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

void
malstrcpy(struct buf *dest, void *src, size_t size)
{
	if (dest == NULL || dest->ptr != NULL || src == NULL || size == 0) return;
	dest->ptr = malloc(sizeof(char) * size);
	if (dest->ptr == NULL) return;
	dest->len = size;
	memcpy(dest->ptr, src, sizeof(char) * size);
}

void
free_string(struct buf *dest)
{
	if (dest == NULL) return;
	if (dest->ptr != NULL) { free(dest->ptr); dest->ptr = NULL; }
	dest->len = 0;
}

void
free_string_ptr(struct buf *dest)
{
	if (dest == NULL) return;
	if (dest->ptr != NULL) free(dest->ptr);
	free(dest);
}

void
cat_strings(struct buf *dest, struct buf *src)
{
	dest->len += src->len;
	dest->ptr = realloc(dest->ptr, dest->len);
	strcat(dest->ptr, src->ptr);
}

void
cat_string_cstr(struct buf *dest, char *src)
{
	dest->len += strlen(src);
	dest->ptr = realloc(dest->ptr, dest->len);
	strcat(dest->ptr, src);
}

int
is_bufs_equal(struct buf *str1, struct buf *str2)
{
	if (str1 == NULL || str2 == NULL) return 0;
	if (str1->ptr == NULL || str2->ptr == NULL) return 0;
	if (str1->len != str2->len) return 0;
	for (int i = 0; i < str1->len; ++i) {
		if (str1->ptr[i] != str2->ptr[i]) return 0;
	}
	return 1;
}
