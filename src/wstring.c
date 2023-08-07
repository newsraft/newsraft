#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

// Note to the future.
// When allocating memory, we request more resources than necessary to reduce
// the number of further realloc calls to expand wstring buffer.

static inline bool
wstr_set(struct wstring **dest, const wchar_t *src_ptr, size_t src_len, size_t src_lim)
{
	if (*dest == NULL) {
		struct wstring *wstr = malloc(sizeof(struct wstring));
		if (wstr == NULL) {
			FAIL("Not enough memory to create wstring!");
			return false;
		}
		wstr->ptr = malloc(sizeof(wchar_t) * (src_lim + 1));
		if (wstr->ptr == NULL) {
			FAIL("Not enough memory to populate wstring!");
			free(wstr);
			return false;
		}
		if (src_ptr != NULL && src_len > 0) {
			memcpy(wstr->ptr, src_ptr, sizeof(wchar_t) * src_len);
		}
		*(wstr->ptr + src_len) = '\0';
		wstr->len = src_len;
		wstr->lim = src_lim;
		*dest = wstr;
	} else {
		if (src_lim > (*dest)->lim) {
			wchar_t *tmp = realloc((*dest)->ptr, sizeof(wchar_t) * (src_lim + 1));
			if (tmp == NULL) {
				FAIL("Not enough memory to set wstring!");
				return false;
			}
			(*dest)->ptr = tmp;
			(*dest)->lim = src_lim;
		}
		if (src_ptr != NULL && src_len > 0) {
			memcpy((*dest)->ptr, src_ptr, sizeof(wchar_t) * src_len);
		}
		*((*dest)->ptr + src_len) = '\0';
		(*dest)->len = src_len;
	}
	return true;
}

struct wstring *
wcrtes(size_t desired_capacity)
{
	struct wstring *wstr = NULL;
	return wstr_set(&wstr, NULL, 0, desired_capacity) == true ? wstr : NULL;
}

struct wstring *
wcrtas(const wchar_t *src_ptr, size_t src_len)
{
	struct wstring *wstr = NULL;
	return wstr_set(&wstr, src_ptr, src_len, src_len) == true ? wstr : NULL;
}

bool
wcpyas(struct wstring *dest, const wchar_t *src_ptr, size_t src_len)
{
	return wstr_set(&dest, src_ptr, src_len, src_len);
}

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
	if (src_ptr != NULL && src_len > 0) {
		memcpy(dest->ptr + dest->len, src_ptr, sizeof(wchar_t) * src_len);
	}
	*(dest->ptr + new_len) = L'\0';
	dest->len = new_len;
	return true;
}

bool
wcatss(struct wstring *dest, const struct wstring *src)
{
	return wcatas(dest, src->ptr, src->len);
}

bool
wcatcs(struct wstring *dest, wchar_t c)
{
	return wcatas(dest, &c, 1);
}

bool
increase_wstring_size(struct wstring *dest, size_t expansion)
{
	size_t new_lim = dest->lim + expansion;
	wchar_t *new_ptr = realloc(dest->ptr, sizeof(wchar_t) * (new_lim + 1));
	if (new_ptr == NULL) {
		return false;
	}
	dest->ptr = new_ptr;
	dest->lim = new_lim;
	return true;
}

void
empty_wstring(struct wstring *dest)
{
	dest->len = 0;
	dest->ptr[0] = '\0';
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
	if (wstr->len != 0) {
		size_t left_edge = 0;
		while (left_edge < wstr->len && ISWIDEWHITESPACE(*(wstr->ptr + left_edge))) {
			++left_edge;
		}
		while (left_edge < wstr->len && ISWIDEWHITESPACE(*(wstr->ptr + wstr->len - 1))) {
			wstr->len -= 1;
		}
		if (left_edge != 0) {
			wstr->len -= left_edge;
			for (size_t i = 0; i < wstr->len; ++i) {
				*(wstr->ptr + i) = *(wstr->ptr + i + left_edge);
			}
		}
		*(wstr->ptr + wstr->len) = L'\0';
	}
}

struct string *
convert_wstring_to_string(const struct wstring *src)
{
	struct string *str = crtes(src->len * 5);
	if (str == NULL) {
		return NULL;
	}
	str->len = wcstombs(str->ptr, src->ptr, str->lim + 1);
	if (str->len == (size_t)-1) {
		free_string(str);
		return NULL;
	}
	str->ptr[str->len] = '\0';
	return str;
}

struct string *
convert_warray_to_string(const wchar_t *src_ptr, size_t src_len)
{
	struct wstring *wstr = wcrtas(src_ptr, src_len);
	if (wstr == NULL) {
		return NULL;
	}
	struct string *str = convert_wstring_to_string(wstr);
	free_wstring(wstr);
	return str;
}
