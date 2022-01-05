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
// On success returns true.
// On memory shortage returns false.
bool
wcpyas(struct wstring *dest, const wchar_t *src_ptr, size_t src_len)
{
	if (src_len > dest->lim) {
		// Multiply by 2 to decrease number of further realloc calls.
		wchar_t *temp = realloc(dest->ptr, sizeof(wchar_t) * (src_len * 2 + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for copying array to wstring!");
			return false;
		}
		dest->ptr = temp;
		dest->lim = src_len * 2;
	}
	memcpy(dest->ptr, src_ptr, sizeof(wchar_t) * src_len);
	*(dest->ptr + src_len) = L'\0';
	dest->len = src_len;
	return true;
}

// Copy wstring to wstring.
bool
wcpyss(struct wstring *dest, const struct wstring *src)
{
	return wcpyas(dest, src->ptr, src->len);
}

// Concatenate array to wstring.
// On success returns true.
// On memory shortage returns false.
bool
wcatas(struct wstring *dest, const wchar_t *src_ptr, size_t src_len)
{
	size_t new_len = dest->len + src_len;
	if (new_len > dest->lim) {
		// Multiply by 2 to decrease number of further realloc calls.
		wchar_t *temp = realloc(dest->ptr, sizeof(wchar_t) * (new_len * 2 + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for concatenating array to wstring!");
			return false;
		}
		dest->ptr = temp;
		dest->lim = new_len * 2;
	}
	memcpy(dest->ptr + dest->len, src_ptr, sizeof(wchar_t) * src_len);
	*(dest->ptr + new_len) = L'\0';
	dest->len = new_len;
	return true;
}

// Concatenate wstring to wstring.
bool
wcatss(struct wstring *dest, const struct wstring *src)
{
	return wcatas(dest, src->ptr, src->len);
}

// Concatenate character to wstring.
// On success returns true.
// On memory shortage returns false.
bool
wcatcs(struct wstring *dest, wchar_t c)
{
	size_t new_len = dest->len + 1;
	if (new_len > dest->lim) {
		// Multiply by 2 to decrease number of further realloc calls.
		wchar_t *temp = realloc(dest->ptr, sizeof(wchar_t) * (new_len * 2 + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for concatenating character to wstring!");
			return false;
		}
		dest->ptr = temp;
		dest->lim = new_len * 2;
	}
	*(dest->ptr + dest->len) = c;
	dest->len = new_len;
	*(dest->ptr + dest->len) = L'\0';
	return true;
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

void
strip_whitespace_from_wstring(struct wstring *wstr)
{
	if (wstr->len == 0) {
		return;
	}

	size_t left_edge = 0, right_edge = wstr->len - 1;
	while ((*(wstr->ptr + left_edge) == L' '   ||
	        *(wstr->ptr + left_edge) == L'\t'  ||
	        *(wstr->ptr + left_edge) == L'\r'  ||
	        *(wstr->ptr + left_edge) == L'\n') &&
	       left_edge <= right_edge)
	{
		++left_edge;
	}
	while ((*(wstr->ptr + right_edge) == L' '   ||
	        *(wstr->ptr + right_edge) == L'\t'  ||
	        *(wstr->ptr + right_edge) == L'\r'  ||
	        *(wstr->ptr + right_edge) == L'\n') &&
	       right_edge >= left_edge)
	{
		--right_edge;
	}

	if ((left_edge == 0) && (right_edge == (wstr->len - 1))) {
		return;
	}

	if (right_edge < left_edge) {
		*(wstr->ptr + 0) = L'\0';
		wstr->len = 0;
		return;
	}

	size_t stripped_wstring_len = right_edge - left_edge + 1;
	for (size_t i = 0; i < stripped_wstring_len; ++i) {
		*(wstr->ptr + i) = *(wstr->ptr + i + left_edge);
	}
	wstr->len = stripped_wstring_len;
	*(wstr->ptr + stripped_wstring_len) = L'\0';
}

// On failure retruns NULL.
struct string *
convert_wstring_to_string(const struct wstring *src)
{
	struct string *str = malloc(sizeof(struct string));
	if (str == NULL) {
		return NULL;
	}
	str->len = wcstombs(NULL, src->ptr, 0);
	if (str->len == (size_t)-1) {
		free(str);
		return NULL;
	}
	str->ptr = malloc(sizeof(char) * (str->len + 1));
	if (str->ptr == NULL) {
		free(str);
		return NULL;
	}
	if (wcstombs(str->ptr, src->ptr, str->len + 1) == (size_t)-1) {
		free(str->ptr);
		free(str);
		return NULL;
	}
	str->ptr[str->len] = '\0';
	str->lim = str->len;
	return str;
}
