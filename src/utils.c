#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "feedeater.h"

void
malstrcpy(struct string *dest, void *src, size_t size)
{
	if (dest == NULL || dest->ptr != NULL || src == NULL || size == 0) return;
	dest->ptr = malloc(sizeof(char) * size);
	if (dest->ptr == NULL) return;
	dest->len = size;
	strncpy(dest->ptr, src, sizeof(char) * size);
}
