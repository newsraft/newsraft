#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

// Note to the future.
// When allocating memory, we request more resources than necessary to reduce
// the number of further realloc calls to expand wstring buffer.

void
wstr_set(struct wstring **dest, const wchar_t *src_ptr, size_t src_len, size_t src_lim)
{
	if (*dest == NULL) {
		struct wstring *wstr = newsraft_malloc(sizeof(struct wstring));
		wstr->ptr = newsraft_malloc(sizeof(wchar_t) * (src_lim + 1));
		if (src_ptr != NULL && src_len > 0) {
			memcpy(wstr->ptr, src_ptr, sizeof(wchar_t) * src_len);
		}
		*(wstr->ptr + src_len) = '\0';
		wstr->len = src_len;
		wstr->lim = src_lim;
		*dest = wstr;
	} else {
		if (src_lim > (*dest)->lim) {
			(*dest)->ptr = newsraft_realloc((*dest)->ptr, sizeof(wchar_t) * (src_lim + 1));
			(*dest)->lim = src_lim;
		}
		if (src_ptr != NULL && src_len > 0) {
			memcpy((*dest)->ptr, src_ptr, sizeof(wchar_t) * src_len);
		}
		*((*dest)->ptr + src_len) = '\0';
		(*dest)->len = src_len;
	}
}

struct wstring *
wcrtes(size_t desired_capacity)
{
	struct wstring *wstr = NULL;
	wstr_set(&wstr, NULL, 0, desired_capacity);
	return wstr;
}

struct wstring *
wcrtas(const wchar_t *src_ptr, size_t src_len)
{
	struct wstring *wstr = NULL;
	wstr_set(&wstr, src_ptr, src_len, src_len);
	return wstr;
}

void
wcatas(struct wstring *dest, const wchar_t *src_ptr, size_t src_len)
{
	size_t new_len = dest->len + src_len;
	if (new_len > dest->lim) {
		size_t new_lim = new_len * 2 + 67;
		dest->ptr = newsraft_realloc(dest->ptr, sizeof(wchar_t) * (new_lim + 1));
		dest->lim = new_lim;
	}
	if (src_ptr != NULL && src_len > 0) {
		memcpy(dest->ptr + dest->len, src_ptr, sizeof(wchar_t) * src_len);
	}
	*(dest->ptr + new_len) = L'\0';
	dest->len = new_len;
}

void
wcatss(struct wstring *dest, const struct wstring *src)
{
	wcatas(dest, src->ptr, src->len);
}

void
wcatcs(struct wstring *dest, wchar_t c)
{
	wcatas(dest, &c, 1);
}

void
make_sure_there_is_enough_space_in_wstring(struct wstring *dest, size_t need_space)
{
	if (need_space > dest->lim - dest->len) {
		const size_t new_lim = dest->len + need_space;
		dest->ptr = newsraft_realloc(dest->ptr, sizeof(wchar_t) * (new_lim + 1));
		dest->lim = new_lim;
	}
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
