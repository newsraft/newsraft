#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

struct wstring *
create_wstring(const wchar_t *src, size_t len)
{
	struct wstring *wstr = malloc(sizeof(struct wstring));
	if (wstr == NULL) {
		FAIL("Not enough memory for wstring structure!");
		return NULL;
	}
	wstr->ptr = malloc(sizeof(wchar_t) * (len + 1));
	if (wstr->ptr == NULL) {
		FAIL("Not enough memory for wstring pointer!");
		free(wstr);
		return NULL;
	}
	if (src != NULL) {
		if (len != 0) {
			memcpy(wstr->ptr, src, sizeof(wchar_t) * len);
		}
		wstr->len = len;
	} else {
		wstr->len = 0;
	}
	wstr->lim = len;
	*(wstr->ptr + wstr->len) = L'\0';
	return wstr;
}

struct wstring *
create_empty_wstring(void)
{
	return create_wstring(NULL, 0);
}

// Copy array to wstring.
// On success returns 0.
// On failure returns non-zero.
int
wcpyas(struct wstring *dest, const wchar_t *src_ptr, size_t src_len)
{
	if (src_len > dest->lim) {
		// Multiply by 2 to decrease number of further realloc calls.
		wchar_t *temp = realloc(dest->ptr, sizeof(wchar_t) * (src_len * 2 + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for copying array to wstring!");
			return 1;
		}
		dest->ptr = temp;
		dest->lim = src_len * 2;
	}
	memcpy(dest->ptr, src_ptr, sizeof(wchar_t) * src_len);
	*(dest->ptr + src_len) = L'\0';
	dest->len = src_len;
	return 0;
}

// Copy wstring to wstring.
int
wcpyss(struct wstring *dest, const struct wstring *src)
{
	return wcpyas(dest, src->ptr, src->len);
}

// Concatenate array to wstring.
// On success returns 0.
// On failure returns non-zero.
int
wcatas(struct wstring *dest, const wchar_t *src_ptr, size_t src_len)
{
	size_t new_len = dest->len + src_len;
	if (new_len > dest->lim) {
		// Multiply by 2 to decrease number of further realloc calls.
		wchar_t *temp = realloc(dest->ptr, sizeof(wchar_t) * (new_len * 2 + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for concatenating array to wstring!");
			return 1;
		}
		dest->ptr = temp;
		dest->lim = new_len * 2;
	}
	memcpy(dest->ptr + dest->len, src_ptr, sizeof(wchar_t) * src_len);
	*(dest->ptr + new_len) = L'\0';
	dest->len = new_len;
	return 0;
}

// Concatenate wstring to wstring.
int
wcatss(struct wstring *dest, const struct wstring *src)
{
	return wcatas(dest, src->ptr, src->len);
}

// Concatenate character to wstring.
// On success returns 0.
// On failure returns non-zero.
int
wcatcs(struct wstring *dest, wchar_t c)
{
	size_t new_len = dest->len + 1;
	if (new_len > dest->lim) {
		// Multiply by 2 to decrease number of further realloc calls.
		wchar_t *temp = realloc(dest->ptr, sizeof(wchar_t) * (new_len * 2 + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for concatenating character to wstring!");
			return 1;
		}
		dest->ptr = temp;
		dest->lim = new_len * 2;
	}
	*(dest->ptr + dest->len) = c;
	dest->len = new_len;
	*(dest->ptr + dest->len) = L'\0';
	return 0;
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
empty_wstring(struct wstring *wstr)
{
	wstr->len = 0;
	*(wstr->ptr + 0) = L'\0';
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
