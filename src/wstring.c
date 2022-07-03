#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

// Note to the future.
// When allocating memory, we request more resources than necessary to reduce
// the number of further realloc calls to expand string string buffer.

// Create wstring out of array.
// On success returns pointer to wstring.
// On memory shortage returns NULL.
struct wstring *
wcrtas(const wchar_t *src_ptr, size_t src_len)
{
	struct wstring *wstr = malloc(sizeof(struct wstring));
	if (wstr == NULL) {
		FAIL("Not enough memory for wstring structure!");
		return NULL;
	}
	size_t new_lim = src_len * 2 + 67;
	wstr->ptr = malloc(sizeof(wchar_t) * (new_lim + 1));
	if (wstr->ptr == NULL) {
		FAIL("Not enough memory for wstring pointer!");
		free(wstr);
		return NULL;
	}
	memcpy(wstr->ptr, src_ptr, sizeof(wchar_t) * src_len);
	*(wstr->ptr + src_len) = L'\0';
	wstr->len = src_len;
	wstr->lim = new_lim;
	return wstr;
}

// Create wstring out of wstring.
struct wstring *
wcrtss(const struct wstring *src)
{
	return wcrtas(src->ptr, src->len);
}

// Create empty wstring.
struct wstring *
wcrtes(void)
{
	return wcrtas(L"", 0);
}

// Copy array to wstring.
// On success returns true.
// On memory shortage returns false.
bool
wcpyas(struct wstring *dest, const wchar_t *src_ptr, size_t src_len)
{
	if (src_len > dest->lim) {
		size_t new_lim = src_len * 2 + 67;
		wchar_t *temp = realloc(dest->ptr, sizeof(wchar_t) * (new_lim + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for copying array to wstring!");
			return false;
		}
		dest->ptr = temp;
		dest->lim = new_lim;
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
		size_t new_lim = new_len * 2 + 67;
		wchar_t *temp = realloc(dest->ptr, sizeof(wchar_t) * (new_lim + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for concatenating array to wstring!");
			return false;
		}
		dest->ptr = temp;
		dest->lim = new_lim;
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
	wchar_t src[1] = {c};
	return wcatas(dest, src, 1);
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
	if (wstr != NULL) {
		free(wstr->ptr);
		free(wstr);
	}
}

void
trim_whitespace_from_wstring(struct wstring *wstr)
{
	if (wstr->len == 0) {
		return;
	}

	size_t left_edge = 0, right_edge = wstr->len - 1;
	while (ISWIDEWHITESPACE(*(wstr->ptr + left_edge)) && left_edge <= right_edge) {
		++left_edge;
	}
	while (ISWIDEWHITESPACE(*(wstr->ptr + right_edge)) && right_edge >= left_edge) {
		--right_edge;
	}

	if ((left_edge != 0) || (right_edge != (wstr->len - 1))) {
		if (right_edge < left_edge) {
			*(wstr->ptr + 0) = L'\0';
			wstr->len = 0;
		} else {
			size_t trimmed_wstring_len = right_edge - left_edge + 1;
			for (size_t i = 0; i < trimmed_wstring_len; ++i) {
				*(wstr->ptr + i) = *(wstr->ptr + i + left_edge);
			}
			wstr->len = trimmed_wstring_len;
			*(wstr->ptr + trimmed_wstring_len) = L'\0';
		}
	}
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
