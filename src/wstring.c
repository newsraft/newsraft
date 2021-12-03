#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

struct wstring *
create_wstring(const wchar_t *src, size_t len)
{
	struct wstring *wstr = malloc(sizeof(struct wstring));
	if (wstr == NULL) {
		FAIL("Not enough memory for wstring creation!");
		return NULL; // failure
	}
	wstr->ptr = malloc(sizeof(wchar_t) * (len + 1));
	if (wstr->ptr == NULL) {
		FAIL("Not enough memory for wstring creation!");
		free(wstr);
		return NULL; // failure
	}
	if (src != NULL) {
		if (len != 0) {
			memcpy(wstr->ptr, src, sizeof(wchar_t) * len);
		}
		wstr->len = len;
		wstr->lim = len;
	} else {
		wstr->len = 0;
		wstr->lim = len;
	}
	*(wstr->ptr + len) = '\0';
	return wstr; // success
}

struct wstring *
create_empty_wstring(void)
{
	return create_wstring(NULL, 0);
}

void
cat_wstring_array(struct wstring *dest, const wchar_t *src_ptr, size_t src_len)
{
	dest->len += src_len;
	if (dest->len > dest->lim) {
		dest->lim = dest->len;
		dest->ptr = realloc(dest->ptr, sizeof(wchar_t) * (dest->lim + 1));
	}
	wcsncat(dest->ptr, src_ptr, src_len);
}

void
cat_wstring_wchar(struct wstring *dest, wchar_t wc)
{
	dest->len += 1;
	if (dest->len > dest->lim) {
		dest->lim = dest->len;
		dest->ptr = realloc(dest->ptr, sizeof(wchar_t) * (dest->lim + 1));
	}
	*(dest->ptr + dest->len - 1) = wc;
	*(dest->ptr + dest->len) = L'\0';
}

struct wstring *
convert_string_to_wstring(const struct string *src)
{
	struct wstring *wstr = malloc(sizeof(struct wstring));
	if (wstr == NULL) {
		return NULL; // failure
	}
	wstr->len = mbstowcs(NULL, src->ptr, 0);
	if (wstr->len == (size_t)-1) {
		free(wstr);
		return NULL; // failure
	}
	wstr->ptr = malloc(sizeof(wchar_t) * (wstr->len + 1));
	if (wstr->ptr == NULL) {
		free(wstr);
		return NULL; // failure
	}
	if (mbstowcs(wstr->ptr, src->ptr, wstr->len + 1) == (size_t)-1) {
		free(wstr->ptr);
		free(wstr);
		return NULL; // failure
	}
	wstr->ptr[wstr->len] = L'\0';
	wstr->lim = wstr->len;
	return wstr; // success
}

void
free_wstring(struct wstring *wstr)
{
	if (wstr == NULL) {
		return;
	}
	free(wstr->ptr);
	free(wstr);
}
