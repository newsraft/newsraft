#include <stdlib.h>
#include "newsraft.h"

void *
newsraft_malloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr == NULL) {
		abort();
	}
	return ptr;
}

void *
newsraft_calloc(size_t n, size_t size)
{
	void *ptr = calloc(n, size);
	if (ptr == NULL) {
		abort();
	}
	return ptr;
}

void *
newsraft_realloc(void *ptr, size_t size)
{
	void *new_ptr = realloc(ptr, size);
	if (new_ptr == NULL) {
		abort();
	}
	return new_ptr;
}

void
newsraft_free(void *ptr)
{
	free(ptr);
}
