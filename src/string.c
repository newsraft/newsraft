#include <stdlib.h>
#include <string.h>
#include "feedeater.h"

// Create string out of array.
// On success returns pointer to string.
// On memory shortage returns NULL.
struct string *
crtas(const char *src_ptr, size_t src_len)
{
	struct string *str = malloc(sizeof(struct string));
	if (str == NULL) {
		FAIL("Not enough memory for string structure!");
		return NULL;
	}
	size_t new_lim = src_len * 2; // Multiply by 2 to decrease number of further realloc calls.
	str->ptr = malloc(sizeof(char) * (new_lim + 1));
	if (str->ptr == NULL) {
		FAIL("Not enough memory for string pointer!");
		free(str);
		return NULL;
	}
	memcpy(str->ptr, src_ptr, sizeof(char) * src_len);
	*(str->ptr + src_len) = '\0';
	str->len = src_len;
	str->lim = new_lim;
	return str;
}

// Create string out of string.
struct string *
crtss(const struct string *src)
{
	return crtas(src->ptr, src->len);
}

// Create empty string.
struct string *
crtes(void)
{
	return crtas("", 0);
}

// Copy array to string.
// On success returns true.
// On memory shortage returns false.
bool
cpyas(struct string *dest, const char *src_ptr, size_t src_len)
{
	if (src_len > dest->lim) {
		size_t new_lim = src_len * 2; // Multiply by 2 to decrease number of further realloc calls.
		char *temp = realloc(dest->ptr, sizeof(char) * (new_lim + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for copying array to string!");
			return false;
		}
		dest->ptr = temp;
		dest->lim = new_lim;
	}
	memcpy(dest->ptr, src_ptr, sizeof(char) * src_len);
	*(dest->ptr + src_len) = '\0';
	dest->len = src_len;
	return true;
}

// Copy string to string.
bool
cpyss(struct string *dest, const struct string *src)
{
	return cpyas(dest, src->ptr, src->len);
}

// Concatenate array to string.
// On success returns true.
// On memory shortage returns false.
bool
catas(struct string *dest, const char *src_ptr, size_t src_len)
{
	size_t new_len = dest->len + src_len;
	if (new_len > dest->lim) {
		size_t new_lim = new_len * 2; // Multiply by 2 to decrease number of further realloc calls.
		char *temp = realloc(dest->ptr, sizeof(char) * (new_lim + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for concatenating array to string!");
			return false;
		}
		dest->ptr = temp;
		dest->lim = new_lim;
	}
	memcpy(dest->ptr + dest->len, src_ptr, sizeof(char) * src_len);
	*(dest->ptr + new_len) = '\0';
	dest->len = new_len;
	return true;
}

// Concatenate string to string.
bool
catss(struct string *dest, const struct string *src)
{
	return catas(dest, src->ptr, src->len);
}

// Concatenate character to string.
// On success returns true.
// On memory shortage returns false.
bool
catcs(struct string *dest, char c)
{
	size_t new_len = dest->len + 1;
	if (new_len > dest->lim) {
		size_t new_lim = new_len * 2; // Multiply by 2 to decrease number of further realloc calls.
		char *temp = realloc(dest->ptr, sizeof(char) * (new_lim + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for concatenating character to string!");
			return false;
		}
		dest->ptr = temp;
		dest->lim = new_lim;
	}
	*(dest->ptr + dest->len) = c;
	dest->len = new_len;
	*(dest->ptr + dest->len) = '\0';
	return true;
}

void
empty_string(struct string *str)
{
	str->len = 0;
	*(str->ptr + 0) = '\0';
}

void
free_string(struct string *str)
{
	if (str == NULL) {
		return;
	}
	free(str->ptr);
	free(str);
}

void
trim_whitespace_from_string(struct string *str)
{
	if (str->len == 0) {
		return;
	}

	size_t left_edge = 0, right_edge = str->len - 1;
	while ((*(str->ptr + left_edge) == ' '   ||
	        *(str->ptr + left_edge) == '\n'  ||
	        *(str->ptr + left_edge) == '\t'  ||
	        *(str->ptr + left_edge) == '\v'  ||
	        *(str->ptr + left_edge) == '\f'  ||
	        *(str->ptr + left_edge) == '\r') &&
	       left_edge <= right_edge)
	{
		++left_edge;
	}
	while ((*(str->ptr + right_edge) == ' '   ||
	        *(str->ptr + right_edge) == '\n'  ||
	        *(str->ptr + right_edge) == '\t'  ||
	        *(str->ptr + right_edge) == '\v'  ||
	        *(str->ptr + right_edge) == '\f'  ||
	        *(str->ptr + right_edge) == '\r') &&
	       right_edge >= left_edge)
	{
		--right_edge;
	}

	if ((left_edge == 0) && (right_edge == (str->len - 1))) {
		return;
	}

	if (right_edge < left_edge) {
		*(str->ptr + 0) = '\0';
		str->len = 0;
		return;
	}

	size_t trimmed_string_len = right_edge - left_edge + 1;
	for (size_t i = 0; i < trimmed_string_len; ++i) {
		*(str->ptr + i) = *(str->ptr + i + left_edge);
	}
	str->len = trimmed_string_len;
	*(str->ptr + trimmed_string_len) = '\0';
}

// On failure retruns NULL.
struct wstring *
convert_string_to_wstring(const struct string *src)
{
	struct wstring *wstr = malloc(sizeof(struct wstring));
	if (wstr == NULL) {
		return NULL;
	}
	wstr->len = mbstowcs(NULL, src->ptr, 0);
	if (wstr->len == (size_t)-1) {
		free(wstr);
		return NULL;
	}
	wstr->ptr = malloc(sizeof(wchar_t) * (wstr->len + 1));
	if (wstr->ptr == NULL) {
		free(wstr);
		return NULL;
	}
	if (mbstowcs(wstr->ptr, src->ptr, wstr->len + 1) == (size_t)-1) {
		free(wstr->ptr);
		free(wstr);
		return NULL;
	}
	wstr->ptr[wstr->len] = L'\0';
	wstr->lim = wstr->len;
	return wstr;
}
