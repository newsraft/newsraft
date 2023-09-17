#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

// Note to the future.
// When allocating memory, we request more resources than necessary to reduce
// the number of further realloc calls to expand string buffer.

static inline bool
str_set(struct string **dest, const char *src_ptr, size_t src_len, size_t src_lim)
{
	if (*dest == NULL) {
		struct string *str = malloc(sizeof(struct string));
		if (str == NULL) {
			FAIL("Not enough memory to create string!");
			return false;
		}
		str->ptr = malloc(sizeof(char) * (src_lim + 1));
		if (str->ptr == NULL) {
			FAIL("Not enough memory to populate string!");
			free(str);
			return false;
		}
		if (src_ptr != NULL && src_len > 0) {
			memcpy(str->ptr, src_ptr, sizeof(char) * src_len);
		}
		*(str->ptr + src_len) = '\0';
		str->len = src_len;
		str->lim = src_lim;
		*dest = str;
	} else {
		if (src_lim > (*dest)->lim) {
			char *tmp = realloc((*dest)->ptr, sizeof(char) * (src_lim + 1));
			if (tmp == NULL) {
				FAIL("Not enough memory to set string!");
				return false;
			}
			(*dest)->ptr = tmp;
			(*dest)->lim = src_lim;
		}
		if (src_ptr != NULL && src_len > 0) {
			memcpy((*dest)->ptr, src_ptr, sizeof(char) * src_len);
		}
		*((*dest)->ptr + src_len) = '\0';
		(*dest)->len = src_len;
	}
	return true;
}

struct string *
crtes(size_t desired_capacity)
{
	struct string *str = NULL;
	return str_set(&str, NULL, 0, desired_capacity) == true ? str : NULL;
}

struct string *
crtas(const char *src_ptr, size_t src_len)
{
	struct string *str = NULL;
	return str_set(&str, src_ptr, src_len, src_len) == true ? str : NULL;
}

struct string *
crtss(const struct string *src)
{
	return crtas(src->ptr, src->len);
}

bool
cpyas(struct string **dest, const char *src_ptr, size_t src_len)
{
	return str_set(dest, src_ptr, src_len, src_len);
}

bool
cpyss(struct string **dest, const struct string *src)
{
	return str_set(dest, src->ptr, src->len, src->len);
}

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
	if (src_ptr != NULL && src_len > 0) {
		memcpy(dest->ptr + dest->len, src_ptr, sizeof(char) * src_len);
	}
	*(dest->ptr + new_len) = '\0';
	dest->len = new_len;
	return true;
}

bool
catss(struct string *dest, const struct string *src)
{
	return catas(dest, src->ptr, src->len);
}

bool
catcs(struct string *dest, char c)
{
	return catas(dest, &c, 1);
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
empty_string(struct string *dest)
{
	dest->len = 0;
	dest->ptr[0] = '\0';
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
	if (str->len != 0) {
		size_t left_edge = 0;
		while (left_edge < str->len && ISWHITESPACE(*(str->ptr + left_edge))) {
			++left_edge;
		}
		while (left_edge < str->len && ISWHITESPACE(*(str->ptr + str->len - 1))) {
			str->len -= 1;
		}
		if (left_edge != 0) {
			str->len -= left_edge;
			for (size_t i = 0; i < str->len; ++i) {
				*(str->ptr + i) = *(str->ptr + i + left_edge);
			}
		}
		*(str->ptr + str->len) = '\0';
	}
}

struct wstring *
convert_string_to_wstring(const struct string *src)
{
	struct wstring *wstr = wcrtes(src->len);
	if (wstr == NULL) {
		return NULL;
	}
	wstr->len = mbstowcs(wstr->ptr, src->ptr, wstr->lim + 1);
	if (wstr->len == (size_t)-1) {
		free_wstring(wstr);
		return NULL;
	}
	wstr->ptr[wstr->len] = L'\0';
	return wstr;
}

struct wstring *
convert_array_to_wstring(const char *src_ptr, size_t src_len)
{
	struct string *str = crtas(src_ptr, src_len);
	if (str == NULL) {
		return NULL;
	}
	struct wstring *wstr = convert_string_to_wstring(str);
	free_string(str);
	return wstr;
}

void
inlinefy_string(struct string *str)
{
	char *i, *j;
	// Replace whitespace with spaces.
	for (i = str->ptr; *i != '\0'; ++i) {
		if (*i == '\n' || *i == '\t' || *i == '\r' || *i == '\f' || *i == '\v') {
			*i = ' ';
		}
	}
	// Replace repeated spaces with a single one.
	for (i = str->ptr; *i != '\0'; ++i) {
		if ((*i == ' ') && (*(i + 1) == ' ')) {
			for (j = i; *j != '\0'; ++j) {
				*j = *(j + 1);
			}
			str->len -= 1;
			i -= 1;
		}
	}
}
