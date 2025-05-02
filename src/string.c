#include <stdlib.h>
#include <string.h>
#include "newsraft.h"

// Note to the future.
// When allocating memory, we request more resources than necessary to reduce
// the number of further realloc calls to expand string buffer.

static inline void
str_set(struct string **dest, const char *src_ptr, size_t src_len, size_t src_lim)
{
	if (*dest == NULL) {
		struct string *str = newsraft_malloc(sizeof(struct string));
		str->ptr = newsraft_malloc(sizeof(char) * (src_lim + 1));
		if (src_ptr != NULL && src_len > 0) {
			memcpy(str->ptr, src_ptr, sizeof(char) * src_len);
			*(str->ptr + src_len) = '\0';
			str->len = src_len;
		} else {
			*(str->ptr) = '\0';
			str->len = 0;
		}
		str->lim = src_lim;
		*dest = str;
	} else {
		if (src_lim > (*dest)->lim) {
			(*dest)->ptr = newsraft_realloc((*dest)->ptr, sizeof(char) * (src_lim + 1));
			(*dest)->lim = src_lim;
		}
		if (src_ptr != NULL && src_len > 0) {
			memcpy((*dest)->ptr, src_ptr, sizeof(char) * src_len);
		}
		*((*dest)->ptr + src_len) = '\0';
		(*dest)->len = src_len;
	}
}

struct string *
crtes(size_t desired_capacity)
{
	struct string *str = NULL;
	str_set(&str, NULL, 0, desired_capacity);
	return str;
}

struct string *
crtas(const char *src_ptr, size_t src_len)
{
	struct string *str = NULL;
	str_set(&str, src_ptr, src_len, src_len);
	return str;
}

struct string *
crtss(const struct string *src)
{
	return crtas(src->ptr, src->len);
}

void
cpyas(struct string **dest, const char *src_ptr, size_t src_len)
{
	str_set(dest, src_ptr, src_len, src_len);
}

void
cpyss(struct string **dest, const struct string *src)
{
	str_set(dest, src->ptr, src->len, src->len);
}

void
catas(struct string *dest, const char *src_ptr, size_t src_len)
{
	size_t new_len = dest->len + src_len;
	if (new_len > dest->lim) {
		size_t new_lim = new_len * 2 + 67;
		dest->ptr = newsraft_realloc(dest->ptr, sizeof(char) * (new_lim + 1));
		dest->lim = new_lim;
	}
	if (src_ptr != NULL && src_len > 0) {
		memcpy(dest->ptr + dest->len, src_ptr, sizeof(char) * src_len);
	}
	*(dest->ptr + new_len) = '\0';
	dest->len = new_len;
}

void
catss(struct string *dest, const struct string *src)
{
	catas(dest, src->ptr, src->len);
}

void
catcs(struct string *dest, char c)
{
	catas(dest, &c, 1);
}

void
make_string_fit_more(struct string **dest, size_t n)
{
	if (*dest == NULL) {
		str_set(dest, NULL, 0, n);
	} else if ((*dest)->len + n > (*dest)->lim) {
		size_t new_lim = ((*dest)->len + n) * 2 + 67;
		(*dest)->ptr = newsraft_realloc((*dest)->ptr, sizeof(char) * (new_lim + 1));
		(*dest)->lim = new_lim;
	}
}

void
str_vappendf(struct string *dest, const char *fmt, va_list args)
{
	char buf[1000];
	int res = vsnprintf(buf, sizeof(buf), fmt, args);
	if (res > 0 && res < (int)sizeof(buf)) {
		catas(dest, buf, res);
	} else {
		catas(dest, "appendix was too big!\n", 22);
	}
}

void
str_appendf(struct string *dest, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	str_vappendf(dest, fmt, args);
	va_end(args);
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
		newsraft_free(str->ptr);
		newsraft_free(str);
	}
}

void
trim_whitespace_from_string(struct string *str)
{
	if (str != NULL && str->len > 0) {
		size_t left_edge = 0;
		while (left_edge < str->len && ISWHITESPACE(str->ptr[left_edge])) {
			left_edge += 1;
		}
		while (left_edge < str->len && ISWHITESPACE(str->ptr[str->len - 1])) {
			str->len -= 1;
		}
		if (left_edge > 0) {
			str->len -= left_edge;
			for (size_t i = 0; i < str->len; ++i) {
				str->ptr[i] = str->ptr[i + left_edge];
			}
		}
		str->ptr[str->len] = '\0';
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
remove_start_of_string(struct string *str, size_t size)
{
	if (size >= str->len) {
		empty_string(str);
	} else {
		for (size_t i = 0; (i + size) < str->len; ++i) {
			str->ptr[i] = str->ptr[i + size];
		}
		str->len -= size;
		str->ptr[str->len] = '\0';
	}
}

void
inlinefy_string(struct string *str)
{
	// Replace multiple whitespace with a single space.
	char *dest = str->ptr;
	char c = '\0';
	for (const char *s = str->ptr; *s != '\0'; ++s) {
		if (ISWHITESPACE(*s)) {
			if (c == ' ') // previous character was whitespace
				continue;
			c = ' ';
		} else {
			c = *s;
		}
		*dest = c;
		++dest;
	}
	*dest = '\0';
	str->len = dest - str->ptr;
}

void
newsraft_simple_hash(struct string **dest, const char *src)
{
	uint64_t hash = 14695981039346656037LLU;
	for (const char *i = src; *i != '\0'; ++i) {
		hash = (hash ^ *i) * 1099511628211LLU;
	}
	char out[64];
	for (size_t i = 0; i < sizeof(out); ++i) {
		out[i] = 32 + hash % 95;
		hash = (hash ^ ((hash << 39) | (hash >> 25))) * 1099511628211LLU;
	}
	cpyas(dest, out, sizeof(out));
}

struct string *
newsraft_base64_encode(const uint8_t *data, size_t size)
{
	static char base64_encoding_table[] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', '+', '/'
	};
	struct string *out = crtes(size * 4);
	for (size_t i = 0; i < size;) {
		uint32_t octet_a = i < size ? data[i++] : 0;
		uint32_t octet_b = i < size ? data[i++] : 0;
		uint32_t octet_c = i < size ? data[i++] : 0;
		uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
		catcs(out, base64_encoding_table[(triple >> 3 * 6) & 0x3F]);
		catcs(out, base64_encoding_table[(triple >> 2 * 6) & 0x3F]);
		catcs(out, base64_encoding_table[(triple >> 1 * 6) & 0x3F]);
		catcs(out, base64_encoding_table[(triple >> 0 * 6) & 0x3F]);
	}
	switch (size % 3) {
		case 1: if (out->len >= 2) out->ptr[out->len - 2] = '='; // fall through
		case 2: if (out->len >= 1) out->ptr[out->len - 1] = '='; // fall through
	}
	return out;
}
