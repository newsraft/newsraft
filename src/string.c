#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

// Note to the future.
// When allocating memory, we request more resources than necessary to reduce
// the number of further realloc calls to expand string buffer.

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
	size_t new_lim = src_len * 2 + 67;
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
		size_t new_lim = src_len * 2 + 67;
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
		size_t new_lim = new_len * 2 + 67;
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
	char src[1] = {c};
	return catas(dest, src, 1);
}

bool
crtas_or_cpyas(struct string **dest, const char *src_ptr, size_t src_len)
{
	if (*dest != NULL) {
		return cpyas(*dest, src_ptr, src_len);
	}
	*dest = crtas(src_ptr, src_len);
	if (*dest == NULL) {
		return false;
	}
	return true;
}

bool
crtss_or_cpyss(struct string **dest, const struct string *src)
{
	if (*dest != NULL) {
		return cpyss(*dest, src);
	}
	*dest = crtss(src);
	if (*dest == NULL) {
		return false;
	}
	return true;
}

bool
string_vprintf(struct string *dest, const char *format, va_list args)
{
	// We need a copy of args because first call to vsnprintf screws original
	// argument list and we need to call vsnprintf after that.
	va_list args_copy;
	va_copy(args_copy, args);
	int required_length = vsnprintf(dest->ptr, 0, format, args_copy);
	va_end(args_copy);
	if (required_length < 0) {
		return false;
	}
	// We already know that integer is positive, so it is safe to cast it to unsigned integer.
	if ((size_t)required_length > dest->lim) {
		size_t new_lim = (size_t)required_length * 2 + 67;
		char *temp = realloc(dest->ptr, sizeof(char) * (new_lim + 1));
		if (temp == NULL) {
			FAIL("Not enough memory for printing to string!");
			return false;
		}
		dest->ptr = temp;
		dest->lim = new_lim;
	}
	required_length = vsnprintf(dest->ptr, required_length + 1, format, args);
	if (required_length < 0) {
		empty_string(dest);
		return false;
	}
	dest->len = (size_t)required_length;
	*(dest->ptr + dest->len) = '\0';
	return true;
}

bool
string_printf(struct string *dest, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	bool result = string_vprintf(dest, format, args);
	va_end(args);
	return result;
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
	if (str != NULL) {
		free(str->ptr);
		free(str);
	}
}

void
trim_whitespace_from_string(struct string *str)
{
	if (str->len == 0) {
		return;
	}

	size_t left_edge = 0, right_edge = str->len - 1;
	while (ISWHITESPACE(*(str->ptr + left_edge)) && left_edge <= right_edge) {
		++left_edge;
	}
	while (ISWHITESPACE(*(str->ptr + right_edge)) && right_edge >= left_edge) {
		--right_edge;
	}

	if ((left_edge != 0) || (right_edge != (str->len - 1))) {
		if (right_edge < left_edge) {
			*(str->ptr + 0) = '\0';
			str->len = 0;
		} else {
			size_t trimmed_string_len = right_edge - left_edge + 1;
			for (size_t i = 0; i < trimmed_string_len; ++i) {
				*(str->ptr + i) = *(str->ptr + i + left_edge);
			}
			str->len = trimmed_string_len;
			*(str->ptr + trimmed_string_len) = '\0';
		}
	}
}

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

void
inlinify_string(struct string *title)
{
	char *i, *j;
	// Replace newlines and tabs with spaces.
	for (i = title->ptr; *i != '\0'; ++i) {
		if ((*i == '\n') || (*i == '\t')) {
			*i = ' ';
		}
	}
	// Replace repeated spaces with a single one.
	for (i = title->ptr; *i != '\0'; ++i) {
		if ((*i == ' ') && (*(i + 1) == ' ')) {
			for (j = i; *j != '\0'; ++j) {
				*j = *(j + 1);
			}
			title->len -= 1;
			i -= 1;
		}
	}
}
